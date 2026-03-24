#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

namespace {

struct Options {
    int games = 8;
    int width = 20;
    int height = 20;
    int maxSteps = 600;
    int threads = 0;
    bool remainIndex = true;
    std::vector<std::string> bots = {"XiaruizeBot", "GcBot"};
};

struct BotStats {
    int wins = 0;
    int survivalCount = 0;
    long long totalRank = 0;
    long long totalArmy = 0;
    long long totalLand = 0;
};

struct GameResult {
    int gameNumber = 0;
    int steps = 0;
    bool stepLimitReached = false;
    std::string winnerName;
    std::vector<BotStats> statsDelta;
};

struct WinRateSummary {
    double rate = 0.0;
    double lowerBound = 0.0;
    double upperBound = 0.0;
};

using TableRow = std::vector<std::string>;

struct SummaryRow {
    int wins = 0;
    TableRow cells;
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

    constexpr double kWilsonZ95 = 1.959963984540054;
    constexpr double z2 = kWilsonZ95 * kWilsonZ95;

    const double n = static_cast<double>(totalGames);
    const double p = static_cast<double>(wins) / n;
    const double denominator = 1.0 + z2 / n;
    const double center = (p + z2 / (2.0 * n)) / denominator;
    const double margin = kWilsonZ95 *
                          std::sqrt((p * (1.0 - p) + z2 / (4.0 * n)) / n) /
                          denominator;

    return {
        p,
        std::clamp(center - margin, 0.0, 1.0),
        std::clamp(center + margin, 0.0, 1.0),
    };
}

std::string formatPercent(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2)
        << std::clamp(value, 0.0, 1.0) * 100.0 << '%';
    return out.str();
}

std::string alignTextRight(const std::string& text, std::size_t width) {
    std::ostringstream out;
    out << std::right << std::setw(static_cast<int>(width)) << text;
    return out.str();
}

std::string formatFixed(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
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
        std::cout << ' ';
        if (leftAligned[i]) {
            std::cout << std::left;
        } else {
            std::cout << std::right;
        }
        std::cout << std::setw(static_cast<int>(widths[i])) << row[i] << " |";
    }
    std::cout << std::left;
    std::cout << '\n';
}

void printSummaryTable(const Options& options, const std::vector<BotStats>& stats) {
    const TableRow header = {"Bot",      "Wins",    "Win Rate", "95% CI",
                             "Avg Rank", "Survived", "Avg Army", "Avg Land"};
    const std::vector<bool> leftAligned = {true, false, false, true,
                                           false, false, false, false};

    std::vector<SummaryRow> rows;
    rows.reserve(options.bots.size());
    for (std::size_t i = 0; i < options.bots.size(); ++i) {
        const BotStats& botStats = stats[i];
        const WinRateSummary winRate =
            calculateWinRateSummary(botStats.wins, options.games);
        const std::string lowerBound =
            alignTextRight(formatPercent(winRate.lowerBound), 7);
        const std::string upperBound =
            alignTextRight(formatPercent(winRate.upperBound), 7);

        rows.push_back({
            botStats.wins,
            {
                options.bots[i],
                std::to_string(botStats.wins),
                formatPercent(winRate.rate),
                "[" + lowerBound + ", " + upperBound + "]",
                formatFixed(static_cast<double>(botStats.totalRank) /
                            options.games),
                std::to_string(botStats.survivalCount),
                formatFixed(static_cast<double>(botStats.totalArmy) /
                            options.games),
                formatFixed(static_cast<double>(botStats.totalLand) /
                            options.games),
            },
        });
    }

    std::stable_sort(rows.begin(), rows.end(),
                     [](const SummaryRow& lhs, const SummaryRow& rhs) {
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

void printUsage() {
    std::cout
        << "Usage: LocalGen-bot-simulator [options]\n"
        << "  --games N          Number of matches to run (default: 8)\n"
        << "  --width N          Random map width (default: 20)\n"
        << "  --height N         Random map height (default: 20)\n"
        << "  --threads N        CPU worker threads (default: auto)\n"
        << "  --steps N          Maximum half-turn steps per game (default: "
           "600)\n"
        << "  --shuffle          Randomize player index mapping in simulator\n"
        << "  --bots A B ...     Bot names to simulate (default: XiaruizeBot "
           "GcBot)\n";
}

bool parseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games" || arg == "--width" || arg == "--height" ||
            arg == "--steps" || arg == "--threads") {
            if (i + 1 >= argc) return false;
            int value = 0;
            if (!parsePositiveInt(argv[++i], value)) return false;
            if (arg == "--games")
                options.games = value;
            else if (arg == "--width")
                options.width = value;
            else if (arg == "--height")
                options.height = value;
            else if (arg == "--threads")
                options.threads = value;
            else
                options.maxSteps = value;
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

std::vector<RankItem> rankByPlayer(const std::vector<RankItem>& rank,
                                   std::size_t playerCount) {
    std::vector<RankItem> byPlayer(playerCount);
    for (const RankItem& item : rank) {
        if (item.player >= 0 &&
            item.player < static_cast<index_t>(playerCount)) {
            byPlayer[item.player] = item;
        }
    }
    return byPlayer;
}

std::size_t findBotIndex(const std::vector<std::string>& botNames,
                         const std::string& name) {
    auto it = std::find(botNames.begin(), botNames.end(), name);
    if (it == botNames.end()) return 0;
    return static_cast<std::size_t>(std::distance(botNames.begin(), it));
}

GameResult runSingleGame(const Options& options, int gameNumber) {
    GameResult result;
    result.gameNumber = gameNumber;
    result.statsDelta.resize(options.bots.size());

    Board board = Board::generate(options.width, options.height);

    std::vector<Player*> players;
    std::vector<index_t> teams;
    std::vector<std::string> names;
    players.reserve(options.bots.size());
    teams.reserve(options.bots.size());
    names.reserve(options.bots.size());

    for (std::size_t i = 0; i < options.bots.size(); ++i) {
        const std::string& botName = options.bots[i];
        BasicBot* bot = BotFactory::instance().create(botName);
        if (bot == nullptr) {
            for (Player* player : players) delete player;
            std::ostringstream err;
            err << "Failed to create bot: " << botName;
            throw std::runtime_error(err.str());
        }
        players.push_back(bot);
        teams.push_back(static_cast<index_t>(i));
        names.push_back(botName);
    }

    BasicGame game(options.remainIndex, players, teams, names, board);
    const int initResult = game.init();
    if (initResult != 0) {
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
    const std::vector<RankItem> byPlayer =
        rankByPlayer(finalRank, options.bots.size());

    result.stepLimitReached =
        static_cast<int>(game.getAlivePlayers().size()) > 1 &&
        result.steps >= options.maxSteps;
    if (!finalRank.empty()) {
        result.winnerName = game.getName(finalRank.front().player);
        result.statsDelta[findBotIndex(options.bots, result.winnerName)].wins++;
    }

    for (std::size_t i = 0; i < finalRank.size(); ++i) {
        const std::string playerName = game.getName(finalRank[i].player);
        result.statsDelta[findBotIndex(options.bots, playerName)].totalRank +=
            static_cast<long long>(i) + 1;
    }

    for (int playerID = 0; playerID < static_cast<int>(options.bots.size());
         ++playerID) {
        const std::string playerName = game.getName(playerID);
        const std::size_t botIndex = findBotIndex(options.bots, playerName);
        if (playerID >= 0 && playerID < static_cast<int>(byPlayer.size())) {
            const RankItem& item = byPlayer[playerID];
            BotStats& botStats = result.statsDelta[botIndex];
            botStats.totalArmy += item.army;
            botStats.totalLand += item.land;
            botStats.survivalCount += item.alive ? 1 : 0;
        }
    }

    return result;
}

int detectWorkerCount(const Options& options) {
    const unsigned detected = std::thread::hardware_concurrency();
    const int preferred =
        options.threads > 0 ? options.threads : static_cast<int>(detected);
    return std::max(1, std::min(std::max(1, preferred), options.games));
}

void printGameResult(const GameResult& result) {
    std::cout << "Game " << result.gameNumber << ": ";
    if (!result.winnerName.empty()) {
        std::cout << result.winnerName;
        if (result.stepLimitReached) {
            std::cout << " leads at step limit";
        } else {
            std::cout << " wins";
        }
    }
    std::cout << " after " << result.steps << " half-turns\n";
}

void accumulateStats(std::vector<BotStats>& stats, const GameResult& result) {
    for (std::size_t i = 0; i < stats.size(); ++i) {
        stats[i].wins += result.statsDelta[i].wins;
        stats[i].survivalCount += result.statsDelta[i].survivalCount;
        stats[i].totalRank += result.statsDelta[i].totalRank;
        stats[i].totalArmy += result.statsDelta[i].totalArmy;
        stats[i].totalLand += result.statsDelta[i].totalLand;
    }
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parseArgs(argc, argv, options)) {
        printUsage();
        return 1;
    }

    const auto registeredBots = BotFactory::instance().list();
    std::unordered_map<std::string, bool> registered;
    for (const auto& name : registeredBots) {
        registered[name] = true;
    }
    for (const auto& botName : options.bots) {
        if (!registered.count(botName)) {
            std::cerr << "Unknown bot: " << botName << "\nAvailable bots:";
            for (const auto& name : registeredBots) {
                std::cerr << ' ' << name;
            }
            std::cerr << '\n';
            return 2;
        }
    }

    std::vector<BotStats> stats(options.bots.size());
    const int workerCount = detectWorkerCount(options);

    std::cout << "Running " << options.games << " games on " << options.width
              << 'x' << options.height << " random maps with bots:";
    for (const auto& name : options.bots) std::cout << ' ' << name;
    std::cout << "\nUsing " << workerCount << " CPU worker thread(s).\n\n";

    std::atomic<int> nextGame{1};
    std::atomic<bool> stopRequested{false};
    std::mutex outputMutex;
    std::mutex statsMutex;
    std::mutex errorMutex;
    std::exception_ptr workerError;
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    auto worker = [&]() {
        while (!stopRequested.load()) {
            const int gameNumber = nextGame.fetch_add(1);
            if (gameNumber > options.games) return;

            try {
                GameResult result = runSingleGame(options, gameNumber);
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    printGameResult(result);
                }
                {
                    std::lock_guard<std::mutex> lock(statsMutex);
                    accumulateStats(stats, result);
                }
            } catch (...) {
                {
                    std::lock_guard<std::mutex> lock(errorMutex);
                    if (!workerError) workerError = std::current_exception();
                }
                stopRequested.store(true);
                return;
            }
        }
    };

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
            std::cerr << "Simulation failed: " << ex.what() << '\n';
            return 3;
        }
    }

    std::cout << "\nSummary\n";
    printSummaryTable(options, stats);

    return 0;
}
