/**
 * @file botSimulator.cpp
 *
 * LocalGen Bot Simulator
 * Lightweight CLI for bot-vs-bot evaluation.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"
#include "core/map.hpp"

namespace {

class TimedBot : public BasicBot {
   public:
    explicit TimedBot(BasicBot* inner) : inner_(inner) {}
    ~TimedBot() override { delete inner_; }

    void init(index_t playerId, const GameConstantsPack& constants) override {
        inner_->init(playerId, constants);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        auto start = std::chrono::steady_clock::now();
        inner_->requestMove(boardView, rank);
        auto end = std::chrono::steady_clock::now();
        totalTime_ += std::chrono::duration<double, std::micro>(end - start);
        ++callCount_;
        moveQueue = std::move(inner_->getMoveQueue());
    }

    void onGameEvent(const GameEvent& event) override {
        inner_->onGameEvent(event);
    }

    double totalMicroseconds() const { return totalTime_.count(); }
    long long callCount() const { return callCount_; }

   private:
    BasicBot* inner_;
    std::chrono::duration<double, std::micro> totalTime_{0};
    long long callCount_ = 0;
};

struct Options {
    int games = 8, width = 20, height = 20, maxSteps = 600, threads = 0;
    bool remainIndex = true, silent = false, measureLatency = false;
    std::string mapPath;
    Board customBoard;
    std::vector<std::string> bots = {"XiaruizeBot", "GcBot"};
};

struct BotStats {
    int wins = 0, survivalCount = 0;
    long long totalRank = 0, totalArmy = 0, totalLand = 0;
    int totalKills = 0;
    double totalLatencyMicroseconds = 0.0;
    long long totalLatencyCalls = 0;
};

struct GameResult {
    int gameNumber = 0, steps = 0;
    bool stepLimitReached = false;
    std::string winnerName;
    std::vector<int> ranksByBot;
    std::vector<BotStats> statsDelta;
};

struct WinRateSummary {
    double rate = 0.0, lowerBound = 0.0, upperBound = 0.0;
};

struct ConfidenceInterval {
    double lowerBound = 0.0, upperBound = 0.0;
};

struct TrueSkillRating {
    double mu = 25.0, sigma = 25.0 / 3.0;
};

using TableRow = std::vector<std::string>;

struct SummaryRow {
    double skill = 0.0;
    int wins = 0;
    TableRow cells;
};

constexpr double kConfidenceZ95 = 1.959963984540054;
constexpr double kTrueSkillBeta = (25.0 / 3.0) / 2.0;
constexpr double kTrueSkillTau = (25.0 / 3.0) / 100.0;
constexpr double kTrueSkillMinDelta = 0.0001;

struct Gaussian {
    double pi = 0.0, tau = 0.0;

    Gaussian() = default;

    Gaussian(double mu, double sigma)
        : pi(1.0 / (sigma * sigma)), tau(pi * mu) {}

    double mu() const { return pi != 0.0 ? tau / pi : 0.0; }

    double sigma() const {
        return pi > 0.0 ? std::sqrt(1.0 / pi)
                        : std::numeric_limits<double>::infinity();
    }
};

Gaussian operator*(const Gaussian& lhs, const Gaussian& rhs) {
    Gaussian result;
    result.pi = lhs.pi + rhs.pi;
    result.tau = lhs.tau + rhs.tau;
    return result;
}

Gaussian operator/(const Gaussian& lhs, const Gaussian& rhs) {
    Gaussian result;
    result.pi = lhs.pi - rhs.pi;
    result.tau = lhs.tau - rhs.tau;
    return result;
}

struct Variable : Gaussian {
    std::unordered_map<const void*, Gaussian> messages;

    double set(const Gaussian& value) {
        const double piDelta = std::abs(pi - value.pi);
        const double tauDelta = std::abs(tau - value.tau);
        pi = value.pi;
        tau = value.tau;
        if (std::isinf(piDelta)) return tauDelta;
        return std::max(tauDelta, std::sqrt(piDelta));
    }

    void connect(const void* factor) { messages.emplace(factor, Gaussian{}); }

    double updateMessage(const void* factor, const Gaussian& message) {
        const Gaussian oldMessage = messages[factor];
        messages[factor] = message;
        return set((static_cast<const Gaussian&>(*this) / oldMessage) *
                   message);
    }

    double updateValue(const void* factor, const Gaussian& value) {
        const Gaussian oldMessage = messages[factor];
        messages[factor] = value * oldMessage / static_cast<Gaussian&>(*this);
        return set(value);
    }
};

struct Factor {
    explicit Factor(std::vector<Variable*> variables)
        : vars(std::move(variables)) {
        for (Variable* var : vars) {
            var->connect(this);
        }
    }

    std::vector<Variable*> vars;
};

struct PriorFactor : Factor {
    PriorFactor(Variable* var, const TrueSkillRating& rating, double dynamic)
        : Factor({var}), var(var), rating(rating), dynamic(dynamic) {}

    double down() {
        const double sigma =
            std::sqrt(rating.sigma * rating.sigma + dynamic * dynamic);
        return var->updateValue(this, Gaussian(rating.mu, sigma));
    }

    Variable* var = nullptr;
    TrueSkillRating rating;
    double dynamic = 0.0;
};

struct LikelihoodFactor : Factor {
    LikelihoodFactor(Variable* mean, Variable* value, double variance)
        : Factor({mean, value}), mean(mean), value(value), variance(variance) {}

    double update(Variable* source, Variable* target) {
        const Gaussian msg = *source / source->messages[this];
        const double a = calcA(msg);
        Gaussian message;
        message.pi = a * msg.pi;
        message.tau = a * msg.tau;
        return target->updateMessage(this, message);
    }

    double down() { return update(mean, value); }
    double up() { return update(value, mean); }

    double calcA(const Gaussian& gaussian) const {
        return 1.0 / (1.0 + variance * gaussian.pi);
    }

    Variable* mean = nullptr;
    Variable* value = nullptr;
    double variance = 0.0;
};

struct SumFactor : Factor {
    SumFactor(Variable* sum, std::vector<Variable*> terms,
              std::vector<double> coeffs)
        : Factor(buildVariables(sum, terms)),
          sum(sum),
          terms(std::move(terms)),
          coeffs(std::move(coeffs)) {}

    double down() { return update(sum, terms, coeffs); }

    double up(std::size_t index) {
        std::vector<double> termCoeffs(coeffs.size());
        const double coeff = coeffs[index];
        if (coeff != 0.0) {
            for (std::size_t i = 0; i < coeffs.size(); ++i) {
                termCoeffs[i] = i == index ? 1.0 / coeff : -coeffs[i] / coeff;
            }
        }

        std::vector<Variable*> values = terms;
        values[index] = sum;
        return update(terms[index], values, termCoeffs);
    }

    static std::vector<Variable*> buildVariables(
        Variable* sum, const std::vector<Variable*>& terms) {
        std::vector<Variable*> variables;
        variables.reserve(terms.size() + 1);
        variables.push_back(sum);
        variables.insert(variables.end(), terms.begin(), terms.end());
        return variables;
    }

    double update(Variable* target, const std::vector<Variable*>& values,
                  const std::vector<double>& termCoeffs) {
        double piInverse = 0.0, mean = 0.0;
        for (std::size_t i = 0; i < values.size(); ++i) {
            const Gaussian marginal = *values[i] / values[i]->messages[this];
            mean += termCoeffs[i] * marginal.mu();
            if (std::isinf(piInverse)) continue;
            if (marginal.pi == 0.0) {
                piInverse = std::numeric_limits<double>::infinity();
            } else {
                piInverse += (termCoeffs[i] * termCoeffs[i]) / marginal.pi;
            }
        }

        Gaussian message;
        if (!std::isinf(piInverse) && piInverse > 0.0) {
            message.pi = 1.0 / piInverse;
            message.tau = message.pi * mean;
        }
        return target->updateMessage(this, message);
    }

    Variable* sum = nullptr;
    std::vector<Variable*> terms;
    std::vector<double> coeffs;
};

double standardNormalPdf(double value) {
    static const double kInvSqrtTwoPi = 1.0 / std::sqrt(2.0 * std::acos(-1.0));
    return kInvSqrtTwoPi * std::exp(-0.5 * value * value);
}

double standardNormalCdf(double value) {
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

double vWin(double diff) {
    const double denominator = standardNormalCdf(diff);
    return denominator > 0.0 ? standardNormalPdf(diff) / denominator : -diff;
}

double wWin(double diff) {
    const double v = vWin(diff);
    return std::clamp(v * (v + diff), 0.0, 1.0 - 1e-9);
}

struct TruncateFactor : Factor {
    explicit TruncateFactor(Variable* var) : Factor({var}), var(var) {}

    double up() {
        const Gaussian marginal = *var;
        const Gaussian message = var->messages[this];
        const Gaussian division = marginal / message;
        const double sqrtPi = std::sqrt(division.pi);
        const double diff = division.tau / sqrtPi;
        const double v = vWin(diff);
        const double w = wWin(diff);
        const double denominator = std::max(1e-9, 1.0 - w);

        Gaussian value;
        value.pi = division.pi / denominator;
        value.tau = (division.tau + sqrtPi * v) / denominator;
        return var->updateValue(this, value);
    }

    Variable* var = nullptr;
};

bool parsePositiveInt(const char* text, int& value) {
    char* end = nullptr;
    long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed <= 0) return false;
    value = static_cast<int>(parsed);
    return true;
}

WinRateSummary calculateWinRateSummary(int wins, int totalGames) {
    if (totalGames <= 0) return {};

    constexpr double z2 = kConfidenceZ95 * kConfidenceZ95;

    const double n = static_cast<double>(totalGames);
    const double p = static_cast<double>(wins) / n;
    const double denominator = 1.0 + z2 / n;
    const double center = (p + z2 / (2.0 * n)) / denominator;
    const double margin = kConfidenceZ95 *
                          std::sqrt((p * (1.0 - p) + z2 / (4.0 * n)) / n) /
                          denominator;

    return {
        p,
        std::clamp(center - margin, 0.0, 1.0),
        std::clamp(center + margin, 0.0, 1.0),
    };
}

ConfidenceInterval calculateGaussianConfidenceInterval(double mean,
                                                       double sigma) {
    return {mean - kConfidenceZ95 * sigma, mean + kConfidenceZ95 * sigma};
}

std::string formatFixed(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string formatPercent(double value) {
    return formatFixed(std::clamp(value, 0.0, 1.0) * 100.0) + '%';
}

std::string alignTextRight(const std::string& text, std::size_t width) {
    std::ostringstream out;
    out << std::right << std::setw(static_cast<int>(width)) << text;
    return out.str();
}

void printTableDivider(const std::vector<std::size_t>& widths) {
    std::cout << '+';
    for (std::size_t width : widths) {
        std::cout << std::string(width + 2, '-') << '+';
    }
    std::cout << '\n';
}

void printTableRow(const TableRow& row, const std::vector<std::size_t>& widths,
                   const std::vector<bool>& leftAligned) {
    std::cout << '|';
    for (std::size_t i = 0; i < row.size(); ++i) {
        std::cout << ' ' << (leftAligned[i] ? std::left : std::right)
                  << std::setw(static_cast<int>(widths[i])) << row[i] << " |";
    }
    std::cout << std::left << '\n';
}

void printSummaryTable(const Options& options,
                       const std::vector<BotStats>& stats,
                       const std::vector<TrueSkillRating>& ratings) {
    TableRow header = {"Bot",      "TrueSkill",  "TS 95% CI", "Wins",
                       "Win Rate", "Win 95% CI", "Avg Rank",  "Avg Kill",
                       "Survived", "Avg Army",   "Avg Land"};
    std::vector<bool> leftAligned = {true,  false, true,  false, false, true,
                                     false, false, false, false, false};
    if (options.measureLatency) {
        header.push_back("Avg Latency");
        leftAligned.push_back(false);
    }

    std::vector<SummaryRow> rows;
    rows.reserve(options.bots.size());
    for (std::size_t i = 0; i < options.bots.size(); ++i) {
        const BotStats& botStats = stats[i];
        const WinRateSummary winRate =
            calculateWinRateSummary(botStats.wins, options.games);
        const ConfidenceInterval skillInterval =
            calculateGaussianConfidenceInterval(ratings[i].mu,
                                                ratings[i].sigma);
        TableRow cells = {
            options.bots[i],
            formatFixed(ratings[i].mu),
            "[" + formatFixed(skillInterval.lowerBound) + ", " +
                formatFixed(skillInterval.upperBound) + "]",
            std::to_string(botStats.wins),
            formatPercent(winRate.rate),
            "[" + alignTextRight(formatPercent(winRate.lowerBound), 7) + ", " +
                alignTextRight(formatPercent(winRate.upperBound), 7) + "]",
            formatFixed(static_cast<double>(botStats.totalRank) /
                        options.games),
            formatFixed(static_cast<double>(botStats.totalKills) /
                        options.games),
            std::to_string(botStats.survivalCount),
            formatFixed(static_cast<double>(botStats.totalArmy) /
                        options.games),
            formatFixed(static_cast<double>(botStats.totalLand) /
                        options.games),
        };
        if (options.measureLatency) {
            const double avgLatency = botStats.totalLatencyCalls > 0
                                          ? botStats.totalLatencyMicroseconds /
                                                botStats.totalLatencyCalls
                                          : 0.0;
            cells.push_back(formatFixed(avgLatency) + " us");
        }

        rows.push_back({
            ratings[i].mu,
            botStats.wins,
            std::move(cells),
        });
    }

    std::stable_sort(rows.begin(), rows.end(),
                     [](const SummaryRow& lhs, const SummaryRow& rhs) {
                         if (lhs.skill != rhs.skill)
                             return lhs.skill > rhs.skill;
                         return lhs.wins > rhs.wins;
                     });

    std::vector<std::size_t> widths(header.size(), 0);
    for (std::size_t i = 0; i < header.size(); ++i) {
        widths[i] = header[i].size();
    }
    for (const SummaryRow& row : rows) {
        for (std::size_t i = 0; i < row.cells.size(); ++i) {
            widths[i] = std::max(widths[i], row.cells[i].size());
        }
    }

    printTableDivider(widths);
    printTableRow(header, widths, std::vector<bool>(header.size(), true));
    printTableDivider(widths);
    for (const SummaryRow& row : rows) {
        printTableRow(row.cells, widths, leftAligned);
    }
    printTableDivider(widths);
}

void updateTrueSkillRatings(std::vector<TrueSkillRating>& ratings,
                            const std::vector<int>& ranksByBot) {
    struct OrderedRating {
        std::size_t botIndex = 0;
        int rank = 0;
        TrueSkillRating rating;
    };

    std::vector<OrderedRating> ordered;
    ordered.reserve(ratings.size());
    for (std::size_t i = 0; i < ratings.size(); ++i) {
        ordered.push_back({i, ranksByBot[i], ratings[i]});
    }

    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const OrderedRating& lhs, const OrderedRating& rhs) {
                         if (lhs.rank != rhs.rank) return lhs.rank < rhs.rank;
                         return lhs.botIndex < rhs.botIndex;
                     });

    std::vector<Variable> ratingVars(ordered.size());
    std::vector<Variable> performanceVars(ordered.size());
    std::vector<Variable> teamPerformanceVars(ordered.size());
    std::vector<Variable> teamDiffVars(ordered.size() > 1 ? ordered.size() - 1
                                                          : 0);

    std::vector<PriorFactor> ratingLayer;
    std::vector<LikelihoodFactor> performanceLayer;
    std::vector<SumFactor> teamPerformanceLayer;
    std::vector<SumFactor> teamDiffLayer;
    std::vector<TruncateFactor> truncateLayer;

    ratingLayer.reserve(ordered.size());
    performanceLayer.reserve(ordered.size());
    teamPerformanceLayer.reserve(ordered.size());
    if (ordered.size() > 1) {
        teamDiffLayer.reserve(ordered.size() - 1);
        truncateLayer.reserve(ordered.size() - 1);
    }

    for (std::size_t i = 0; i < ordered.size(); ++i) {
        ratingLayer.emplace_back(&ratingVars[i], ordered[i].rating,
                                 kTrueSkillTau);
        performanceLayer.emplace_back(&ratingVars[i], &performanceVars[i],
                                      kTrueSkillBeta * kTrueSkillBeta);
        teamPerformanceLayer.emplace_back(
            &teamPerformanceVars[i],
            std::vector<Variable*>{&performanceVars[i]},
            std::vector<double>{1.0});
    }

    for (std::size_t i = 0; i + 1 < ordered.size(); ++i) {
        teamDiffLayer.emplace_back(
            &teamDiffVars[i],
            std::vector<Variable*>{&teamPerformanceVars[i],
                                   &teamPerformanceVars[i + 1]},
            std::vector<double>{1.0, -1.0});
        truncateLayer.emplace_back(&teamDiffVars[i]);
    }

    for (PriorFactor& factor : ratingLayer) factor.down();
    for (LikelihoodFactor& factor : performanceLayer) factor.down();
    for (SumFactor& factor : teamPerformanceLayer) factor.down();

    if (!teamDiffLayer.empty()) {
        for (int iteration = 0; iteration < 10; ++iteration) {
            double delta = 0.0;
            if (teamDiffLayer.size() == 1) {
                teamDiffLayer.front().down();
                delta = truncateLayer.front().up();
            } else {
                for (std::size_t i = 0; i + 1 < teamDiffLayer.size(); ++i) {
                    teamDiffLayer[i].down();
                    delta = std::max(delta, truncateLayer[i].up());
                    teamDiffLayer[i].up(1);
                }
                for (std::size_t i = teamDiffLayer.size() - 1; i > 0; --i) {
                    teamDiffLayer[i].down();
                    delta = std::max(delta, truncateLayer[i].up());
                    teamDiffLayer[i].up(0);
                }
            }
            if (delta <= kTrueSkillMinDelta) break;
        }

        teamDiffLayer.front().up(0);
        teamDiffLayer.back().up(1);
    }

    for (SumFactor& factor : teamPerformanceLayer) {
        for (std::size_t i = 0; i < factor.terms.size(); ++i) {
            factor.up(i);
        }
    }
    for (LikelihoodFactor& factor : performanceLayer) factor.up();

    for (std::size_t i = 0; i < ordered.size(); ++i) {
        ratings[ordered[i].botIndex] = {
            ratingVars[i].mu(),
            ratingVars[i].sigma(),
        };
    }
}

void printUsage() {
    std::cout
        << "Usage: LocalGen-bot-simulator [options]\n"
        << "  --games N          Number of matches to run (default: 8)\n"
        << "  --width N          Random map width (default: 20)\n"
        << "  --height N         Random map height (default: 20)\n"
        << "  --map PATH         Use a custom .lgmp (v6) map instead of a "
           "random map\n"
        << "  --threads N        CPU worker threads (default: auto)\n"
        << "  --steps N          Maximum half-turn steps per game (default: "
           "600)\n"
        << "  --silent           Only print the final summary table\n"
        << "  --shuffle          Randomize player index mapping in simulator\n"
        << "  --latency          Measure and report average requestMove() "
           "latency per bot\n"
        << "  --bots A B ...     Bot names to simulate (default: XiaruizeBot "
           "GcBot)\n";
}

bool parseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games" || arg == "--width" || arg == "--height" ||
            arg == "--steps" || arg == "--threads") {
            int* target = arg == "--games"     ? &options.games
                          : arg == "--width"   ? &options.width
                          : arg == "--height"  ? &options.height
                          : arg == "--threads" ? &options.threads
                                               : &options.maxSteps;
            if (i + 1 >= argc || !parsePositiveInt(argv[++i], *target))
                return false;
        } else if (arg == "--map") {
            if (i + 1 >= argc) return false;
            options.mapPath = argv[++i];
        } else if (arg == "--silent") {
            options.silent = true;
        } else if (arg == "--latency") {
            options.measureLatency = true;
        } else if (arg == "--shuffle") {
            options.remainIndex = false;
        } else if (arg == "--bots") {
            options.bots.clear();
            while (i + 1 < argc &&
                   std::string(argv[i + 1]).rfind("--", 0) != 0) {
                options.bots.emplace_back(argv[++i]);
            }
            if (options.bots.empty()) return false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else {
            return false;
        }
    }
    return options.bots.size() >= 2;
}

bool loadCustomMap(Options& options, std::string& errorMessage) {
    if (options.mapPath.empty()) return true;

    const std::filesystem::path mapPath(options.mapPath);
    std::string extension = mapPath.extension().string();
    std::transform(
        extension.begin(), extension.end(), extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (extension != ".lgmp") {
        errorMessage =
            "Only v6 .lgmp maps are supported by --map: " + options.mapPath;
        return false;
    }

    QString qtErrorMessage;
    const MapDocument document =
        openMap_v6(QString::fromStdString(options.mapPath), qtErrorMessage);
    if (!qtErrorMessage.isEmpty()) {
        errorMessage = "Failed to load map '" + options.mapPath +
                       "': " + qtErrorMessage.toStdString();
        return false;
    }

    if (document.board.getWidth() <= 0 || document.board.getHeight() <= 0) {
        errorMessage =
            "Failed to load map '" + options.mapPath + "': empty board.";
        return false;
    }

    options.customBoard = document.board;
    return true;
}

std::size_t botIndexForPlayer(const BasicGame& game, index_t playerId) {
    // Each simulator bot gets a unique team id equal to its original index,
    // so the team id stays as a stable bot mapping even when player slots are
    // shuffled inside BasicGame.
    return static_cast<std::size_t>(game.getTeam(playerId));
}

GameResult runSingleGame(const Options& options, int gameNumber,
                         std::uint64_t mapSeed) {
    GameResult result{gameNumber};
    result.ranksByBot.resize(options.bots.size(),
                             static_cast<int>(options.bots.size()));
    result.statsDelta.resize(options.bots.size());

    Board board =
        options.mapPath.empty()
            ? Board::generate(options.width, options.height,
                              static_cast<int>(options.bots.size()), mapSeed)
            : options.customBoard;

    std::vector<Player*> players;
    std::vector<index_t> teams(options.bots.size());
    players.reserve(options.bots.size());
    std::iota(teams.begin(), teams.end(), 0);

    for (std::size_t i = 0; i < options.bots.size(); ++i) {
        const std::string& botName = options.bots[i];
        BasicBot* bot = BotFactory::instance().create(botName);
        if (options.measureLatency) bot = new TimedBot(bot);
        players.push_back(bot);
    }

    BasicGame game(options.remainIndex, players, teams, options.bots, board);
    if (const int initResult = game.init(); initResult != 0) {
        std::ostringstream err;
        err << "Failed to initialize game " << gameNumber
            << " (spawn error code " << initResult << ")";
        throw std::runtime_error(err.str());
    }

    while (static_cast<int>(game.getAlivePlayers().size()) > 1 &&
           result.steps < options.maxSteps) {
        game.step();
        ++result.steps;
    }

    std::vector<RankItem> finalRank = game.ranklist();

    result.stepLimitReached =
        static_cast<int>(game.getAlivePlayers().size()) > 1 &&
        result.steps >= options.maxSteps;
    const std::size_t winnerIndex =
        botIndexForPlayer(game, finalRank.front().player);
    result.winnerName = options.bots[winnerIndex];
    result.statsDelta[winnerIndex].wins++;

    for (std::size_t i = 0; i < finalRank.size(); ++i) {
        const RankItem& item = finalRank[i];
        const std::size_t botIndex = botIndexForPlayer(game, item.player);
        BotStats& botStats = result.statsDelta[botIndex];
        result.ranksByBot[botIndex] = static_cast<int>(i);
        botStats.totalRank += static_cast<long long>(i) + 1;
        botStats.totalArmy += item.army;
        botStats.totalLand += item.land;
        botStats.totalKills += item.killCount;
        botStats.survivalCount += item.alive;
    }

    if (options.measureLatency) {
        for (std::size_t botIndex = 0; botIndex < players.size(); ++botIndex) {
            const auto* timed = static_cast<TimedBot*>(players[botIndex]);
            result.statsDelta[botIndex].totalLatencyMicroseconds +=
                timed->totalMicroseconds();
            result.statsDelta[botIndex].totalLatencyCalls += timed->callCount();
        }
    }

    return result;
}

int detectWorkerCount(const Options& options) {
    const unsigned detected = std::thread::hardware_concurrency();
    const int preferred =
        options.threads > 0 ? options.threads : static_cast<int>(detected);
    return std::clamp(preferred, 1, options.games);
}

void printGameResult(const GameResult& result) {
    std::cout << "Game " << result.gameNumber << ": " << result.winnerName
              << (result.stepLimitReached ? " leads at step limit" : " wins")
              << " after " << result.steps << " half-turns" << std::endl;
}

void accumulateStats(std::vector<BotStats>& stats, const GameResult& result) {
    for (std::size_t i = 0; i < stats.size(); ++i) {
        auto& total = stats[i];
        const auto& delta = result.statsDelta[i];
        total.wins += delta.wins;
        total.survivalCount += delta.survivalCount;
        total.totalRank += delta.totalRank;
        total.totalArmy += delta.totalArmy;
        total.totalLand += delta.totalLand;
        total.totalKills += delta.totalKills;
        total.totalLatencyMicroseconds += delta.totalLatencyMicroseconds;
        total.totalLatencyCalls += delta.totalLatencyCalls;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Options options;
    if (!parseArgs(argc, argv, options)) {
        printUsage();
        return 1;
    }

    std::string mapError;
    if (!loadCustomMap(options, mapError)) {
        std::cerr << mapError << '\n';
        return 2;
    }

    const auto registeredBots = BotFactory::instance().list();
    const std::unordered_set<std::string> registered(registeredBots.begin(),
                                                     registeredBots.end());
    for (const auto& botName : options.bots) {
        if (registered.count(botName) == 0) {
            std::cerr << "Unknown bot: " << botName << "\nAvailable bots:";
            for (const auto& name : registeredBots) {
                std::cerr << ' ' << name;
            }
            std::cerr << '\n';
            return 3;
        }
    }

    const int workerCount = detectWorkerCount(options);

    if (!options.silent) {
        std::cout << "Running " << options.games << " games on ";
        if (options.mapPath.empty()) {
            std::cout << options.width << 'x' << options.height
                      << " random maps";
        } else {
            std::cout << "custom map " << options.mapPath;
        }
        std::cout << " with bots:";
        for (const auto& name : options.bots) std::cout << ' ' << name;
        std::cout << "\nUsing " << workerCount << " CPU worker thread(s).\n"
                  << std::endl;
    }

    std::atomic<int> nextGame{1};
    std::atomic<bool> stopRequested{false};
    std::vector<GameResult> results(options.games);
    std::vector<std::uint64_t> mapSeeds(options.games);
    std::random_device rd;
    std::seed_seq seedSequence{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    std::mt19937_64 mapSeedGenerator{seedSequence};
    for (std::uint64_t& seed : mapSeeds) {
        seed = mapSeedGenerator();
    }

    std::mutex outputMutex, errorMutex;
    std::exception_ptr workerError;

    auto worker = [&]() {
        while (!stopRequested.load()) {
            const int gameNumber = nextGame.fetch_add(1);
            if (gameNumber > options.games) return;

            try {
                GameResult result = runSingleGame(options, gameNumber,
                                                  mapSeeds[gameNumber - 1]);
                if (!options.silent) {
                    std::lock_guard lock(outputMutex);
                    printGameResult(result);
                }
                results[gameNumber - 1] = std::move(result);
            } catch (...) {
                {
                    std::lock_guard lock(errorMutex);
                    if (!workerError) workerError = std::current_exception();
                }
                stopRequested.store(true);
                return;
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        workers.emplace_back(worker);
    }

    for (std::thread& t : workers) {
        t.join();
    }

    if (workerError) {
        try {
            std::rethrow_exception(workerError);
        } catch (const std::exception& ex) {
            std::cerr << "Simulation failed: " << ex.what() << std::endl;
            return 4;
        }
    }

    std::vector<BotStats> stats(options.bots.size());
    std::vector<TrueSkillRating> ratings(options.bots.size());
    // Replay results in submission order so the online rating update stays
    // deterministic even when games finish on different threads.
    for (const GameResult& result : results) {
        accumulateStats(stats, result);
        updateTrueSkillRatings(ratings, result.ranksByBot);
    }

    if (!options.silent) std::cout << "\nSummary\n";
    printSummaryTable(options, stats, ratings);

    return 0;
}
