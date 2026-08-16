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
#include <mutex>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
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
    std::vector<std::size_t> order;
    std::vector<BotStats> statsDelta;
};

struct WinRateSummary {
    double rate = 0.0, lowerBound = 0.0, upperBound = 0.0;
};

struct ConfidenceInterval {
    double lowerBound = 0.0, upperBound = 0.0;
};

struct OpenSkillRating {
    double mu = 25.0, sigma = 25.0 / 3.0;
};

using TableRow = std::vector<std::string>;

struct SummaryRow {
    double skill = 0.0;
    int wins = 0;
    TableRow cells;
};

constexpr double kConfidenceZ95 = 1.959963984540054;
constexpr double kOpenSkillBeta = 25.0 / 6.0;
constexpr double kOpenSkillTau = 25.0 / 300.0;
constexpr double kOpenSkillKappa = 0.0001;

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
                       const std::vector<OpenSkillRating>& ratings) {
    TableRow header = {"Bot",      "OpenSkill",  "OS 95% CI", "Wins",
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

void updateOpenSkillRatings(std::vector<OpenSkillRating>& ratings,
                            const std::vector<std::size_t>& order) {
    const std::size_t botCount = order.size();
    std::vector<double> sigmas(botCount), strengths(botCount), sumQ(botCount);
    double cSquared = 0.0;
    for (std::size_t i = 0; i < botCount; ++i) {
        const OpenSkillRating& rating = ratings[order[i]];
        sigmas[i] = std::sqrt(rating.sigma * rating.sigma +
                              kOpenSkillTau * kOpenSkillTau);
        cSquared += sigmas[i] * sigmas[i] + kOpenSkillBeta * kOpenSkillBeta;
    }

    const double c = std::sqrt(cSquared);
    double suffixSum = 0.0;
    for (std::size_t i = botCount; i-- > 0;) {
        strengths[i] = std::exp(ratings[order[i]].mu / c);
        suffixSum += strengths[i];
        sumQ[i] = suffixSum;
    }

    for (std::size_t i = 0; i < botCount; ++i) {
        double omegaSum = 0.0, deltaSum = 0.0;
        for (std::size_t q = 0; q <= i; ++q) {
            const double quotient = strengths[i] / sumQ[q];
            omegaSum += (i == q ? 1.0 : 0.0) - quotient;
            deltaSum += quotient * (1.0 - quotient);
        }

        const double variance = sigmas[i] * sigmas[i];
        const double omega = omegaSum * variance / c;
        const double gamma = sigmas[i] / c;
        const double delta = deltaSum * variance / (c * c) * gamma;
        const double mu = ratings[order[i]].mu;
        ratings[order[i]] = {
            mu + omega,
            sigmas[i] * std::sqrt(std::max(1.0 - delta, kOpenSkillKappa)),
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
    const std::size_t botCount = options.bots.size();
    GameResult result{gameNumber};
    result.statsDelta.resize(botCount);
    result.order.resize(botCount);

    Board board = options.mapPath.empty()
                      ? Board::generate(options.width, options.height,
                                        static_cast<int>(botCount), mapSeed)
                      : options.customBoard;

    std::vector<Player*> players;
    std::vector<index_t> teams(botCount);
    players.reserve(botCount);
    std::iota(teams.begin(), teams.end(), 0);

    for (std::size_t i = 0; i < botCount; ++i) {
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

    for (std::size_t i = 0; i < botCount; ++i) {
        const RankItem& item = finalRank[i];
        const std::size_t botIndex = botIndexForPlayer(game, item.player);
        BotStats& botStats = result.statsDelta[botIndex];
        result.order[i] = botIndex;
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
    std::vector<OpenSkillRating> ratings(options.bots.size());
    // Replay results in submission order so the online rating update stays
    // deterministic even when games finish on different threads.
    for (const GameResult& result : results) {
        accumulateStats(stats, result);
        updateOpenSkillRatings(ratings, result.order);
    }

    if (!options.silent) std::cout << "\nSummary\n";
    printSummaryTable(options, stats, ratings);

    return 0;
}
