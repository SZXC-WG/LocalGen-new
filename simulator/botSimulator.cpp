#include <algorithm>
#include <atomic>
#include <cstdlib>
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
    int podiums = 0;
    int survivalCount = 0;
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

bool parsePositiveInt(const char* text, int& value) {
    char* end = nullptr;
    long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed <= 0) return false;
    value = static_cast<int>(parsed);
    return true;
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

    for (std::size_t i = 0; i < finalRank.size() && i < 3; ++i) {
        const std::string playerName = game.getName(finalRank[i].player);
        result.statsDelta[findBotIndex(options.bots, playerName)].podiums++;
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
        stats[i].podiums += result.statsDelta[i].podiums;
        stats[i].survivalCount += result.statsDelta[i].survivalCount;
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
    for (std::size_t i = 0; i < options.bots.size(); ++i) {
        const BotStats& botStats = stats[i];
        std::cout << "- " << options.bots[i] << ": wins=" << botStats.wins
                  << ", podiums=" << botStats.podiums
                  << ", survived=" << botStats.survivalCount << ", avgArmy="
                  << static_cast<double>(botStats.totalArmy) / options.games
                  << ", avgLand="
                  << static_cast<double>(botStats.totalLand) / options.games
                  << '\n';
    }

    return 0;
}
