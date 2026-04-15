#ifndef LGEN_CORE_MAPGEN_HPP
#define LGEN_CORE_MAPGEN_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

inline std::uint32_t makeSeed(std::uint64_t value, std::uint32_t salt) {
    std::uint32_t state =
        static_cast<std::uint32_t>(value ^ (value >> 32) ^ salt);
    return state == 0 ? 1u : state;
}

inline std::uint32_t fastRand(std::uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline int uniformInclusive(std::uint32_t& state, int low, int high) {
    if (high <= low) return low;
    const std::uint32_t span = static_cast<std::uint32_t>(high - low + 1);
    return low + static_cast<int>(fastRand(state) % span);
}

inline int uniformPercentageCount(std::uint32_t& state, int total,
                                  int lowPercent, int highPercent) {
    if (total <= 0 || highPercent <= 0) return 0;
    const int low = std::max(0, total * lowPercent / 100);
    const int high = std::max(low, total * highPercent / 100);
    return uniformInclusive(state, low, high);
}

class Generator {
   public:
    Generator(int width, int height, int spawnCount,
              std::mt19937::result_type seed, bool placeSwamp,
              bool placeLookout, bool placeObservatory, double mountainDensity)
        : width_(width),
          height_(height),
          totalTiles_(width_ > 0 && height_ > 0 ? width_ * height_ : 0),
          outputSpawnCount_(std::clamp(spawnCount, 0, totalTiles_)),
          searchSpawnCount_(std::max(outputSpawnCount_, 1)),
          placeSwamp_(placeSwamp),
          placeLookout_(placeLookout),
          placeObservatory_(placeObservatory),
          mountainDensity_(mountainDensity),
          searchSeed_(makeSeed(static_cast<std::uint64_t>(seed), 0x20260520u)),
          decorateSeed_(
              makeSeed(static_cast<std::uint64_t>(seed), 0xA341316Cu)),
          cityArmySeed_(
              makeSeed(static_cast<std::uint64_t>(seed), 0xC8013EA4u)),
          grid_(totalTiles_, TILE_BLANK),
          bestGrid_(totalTiles_, TILE_BLANK),
          visited_(totalTiles_, 0),
          isCandidate_(totalTiles_, 0),
          customQueue_(totalTiles_),
          maxConnectedComponent_(),
          minDistFps_(totalTiles_),
          curDist_(totalTiles_),
          owner_(totalTiles_),
          area_(searchSpawnCount_),
          sumRow_(searchSpawnCount_),
          sumCol_(searchSpawnCount_),
          count_(searchSpawnCount_),
          spawns_(),
          bestSpawns_(),
          erosionCandidates_(totalTiles_) {
        maxConnectedComponent_.reserve(totalTiles_);
        spawns_.reserve(searchSpawnCount_);
        bestSpawns_.reserve(searchSpawnCount_);
    }

    Board run() {
        if (width_ <= 0 || height_ <= 0) {
            return Board(std::max(height_, 0), std::max(width_, 0));
        }

        generateBasicTerrain();
        decorateBorderMountains();
        placeSwamps();
        return buildBoard();
    }

   private:
    static constexpr int kBlindSearchBudgetMs = 85;
    static constexpr double kComponentPenaltyWeight = 60.0;
    static constexpr int kDirections[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    inline bool inBounds(int row, int col) const {
        return row >= 0 && row < height_ && col >= 0 && col < width_;
    }

    inline long long elapsedMs(
        const std::chrono::steady_clock::time_point& start) const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
            .count();
    }

    void generateBasicTerrain() {
        const auto start = std::chrono::steady_clock::now();

        int targetMountains =
            static_cast<int>(std::lround(totalTiles_ * mountainDensity_));
        targetMountains = std::clamp(targetMountains, 0, totalTiles_ * 3 / 5);

        double baseMargin = 0.06;
        double bestScore = 1e15;

        while (elapsedMs(start) < kBlindSearchBudgetMs || bestSpawns_.empty()) {
            std::fill(grid_.begin(), grid_.end(), TILE_BLANK);

            const double currentMargin =
                baseMargin +
                static_cast<double>(fastRand(searchSeed_) % 50) / 1000.0;
            int initialMountains =
                targetMountains + static_cast<int>(totalTiles_ * currentMargin);
            initialMountains = std::min(initialMountains, totalTiles_ * 3 / 4);

            int currentMountains = 0;
            while (currentMountains < initialMountains) {
                int row = static_cast<int>(fastRand(searchSeed_) % height_);
                int col = static_cast<int>(fastRand(searchSeed_) % width_);
                const int idx = row * width_ + col;
                if (grid_[idx] != TILE_BLANK) continue;

                grid_[idx] = TILE_MOUNTAIN;
                ++currentMountains;

                const int blobSize =
                    1 + static_cast<int>(fastRand(searchSeed_) % 5);
                int curRow = row;
                int curCol = col;
                for (int step = 1;
                     step < blobSize && currentMountains < initialMountains;
                     ++step) {
                    const int dir = static_cast<int>(fastRand(searchSeed_) % 4);
                    curRow += kDirections[dir][0];
                    curCol += kDirections[dir][1];
                    if (!inBounds(curRow, curCol)) break;

                    const int curIdx = curRow * width_ + curCol;
                    if (grid_[curIdx] != TILE_BLANK) break;
                    grid_[curIdx] = TILE_MOUNTAIN;
                    ++currentMountains;
                }
            }
            if (totalTiles_ - currentMountains < searchSpawnCount_) continue;

            if (currentMountains < targetMountains) {
                baseMargin += 0.02;
                if (elapsedMs(start) < kBlindSearchBudgetMs) continue;
            } else if (currentMountains > targetMountains) {
                int candidateTail = 0;
                std::fill(isCandidate_.begin(), isCandidate_.end(), 0);
                for (int i = 0; i < totalTiles_; ++i) {
                    if (grid_[i] != TILE_MOUNTAIN) continue;
                    const int row = i / width_;
                    const int col = i % width_;
                    for (const auto& dir : kDirections) {
                        const int nr = row + dir[0];
                        const int nc = col + dir[1];
                        if (!inBounds(nr, nc)) continue;
                        const int v = nr * width_ + nc;
                        if (grid_[v] != TILE_BLANK) continue;
                        erosionCandidates_[candidateTail++] = i;
                        isCandidate_[i] = 1;
                        break;
                    }
                }

                while (currentMountains > targetMountains &&
                       candidateTail > 0) {
                    const int pick =
                        static_cast<int>(fastRand(searchSeed_) % candidateTail);
                    const int u = erosionCandidates_[pick];
                    erosionCandidates_[pick] =
                        erosionCandidates_[--candidateTail];
                    isCandidate_[u] = 0;

                    if (grid_[u] != TILE_MOUNTAIN) continue;

                    grid_[u] = TILE_BLANK;
                    --currentMountains;
                    const int row = u / width_;
                    const int col = u % width_;
                    for (const auto& dir : kDirections) {
                        const int nr = row + dir[0];
                        const int nc = col + dir[1];
                        if (!inBounds(nr, nc)) continue;
                        const int v = nr * width_ + nc;
                        if (grid_[v] != TILE_MOUNTAIN || isCandidate_[v])
                            continue;
                        isCandidate_[v] = 1;
                        erosionCandidates_[candidateTail++] = v;
                    }
                }
            }

            maxConnectedComponent_.clear();
            int connectedComponentCount = 0;
            std::fill(visited_.begin(), visited_.end(), 0);
            for (int i = 0; i < totalTiles_; ++i) {
                if (grid_[i] != TILE_BLANK || visited_[i]) continue;
                ++connectedComponentCount;

                int head = 0, tail = 0;
                customQueue_[tail++] = i;
                visited_[i] = 1;
                while (head < tail) {
                    const int u = customQueue_[head++];
                    const int row = u / width_;
                    const int col = u % width_;
                    for (const auto& dir : kDirections) {
                        const int nr = row + dir[0];
                        const int nc = col + dir[1];
                        if (!inBounds(nr, nc)) continue;
                        const int v = nr * width_ + nc;
                        if (grid_[v] != TILE_BLANK || visited_[v]) continue;
                        visited_[v] = 1;
                        customQueue_[tail++] = v;
                    }
                }

                if (tail > static_cast<int>(maxConnectedComponent_.size())) {
                    maxConnectedComponent_.assign(customQueue_.begin(),
                                                  customQueue_.begin() + tail);
                }
            }
            if (static_cast<int>(maxConnectedComponent_.size()) <
                searchSpawnCount_)
                continue;

            spawns_.clear();
            spawns_.push_back(
                maxConnectedComponent_[fastRand(searchSeed_) %
                                       maxConnectedComponent_.size()]);
            std::fill(minDistFps_.begin(), minDistFps_.end(),
                      std::numeric_limits<int>::max());

            int head = 0, tail = 0;
            customQueue_[tail++] = spawns_[0];
            minDistFps_[spawns_[0]] = 0;
            while (head < tail) {
                const int u = customQueue_[head++];
                const int row = u / width_;
                const int col = u % width_;
                for (const auto& dir : kDirections) {
                    const int nr = row + dir[0];
                    const int nc = col + dir[1];
                    if (!inBounds(nr, nc)) continue;
                    const int v = nr * width_ + nc;
                    if (grid_[v] != TILE_BLANK ||
                        minDistFps_[v] <= minDistFps_[u] + 1)
                        continue;
                    minDistFps_[v] = minDistFps_[u] + 1;
                    customQueue_[tail++] = v;
                }
            }

            for (int k = 1; k < searchSpawnCount_; ++k) {
                int bestVertex = -1;
                int maxDistance = -1;
                for (int u : maxConnectedComponent_) {
                    if (minDistFps_[u] <= maxDistance) continue;
                    maxDistance = minDistFps_[u];
                    bestVertex = u;
                }
                spawns_.push_back(bestVertex);
                head = 0;
                tail = 0;
                customQueue_[tail++] = bestVertex;
                minDistFps_[bestVertex] = 0;
                while (head < tail) {
                    const int u = customQueue_[head++];
                    const int row = u / width_;
                    const int col = u % width_;
                    for (const auto& dir : kDirections) {
                        const int nr = row + dir[0];
                        const int nc = col + dir[1];
                        if (!inBounds(nr, nc)) continue;
                        const int v = nr * width_ + nc;
                        if (grid_[v] != TILE_BLANK ||
                            minDistFps_[v] <= minDistFps_[u] + 1)
                            continue;
                        minDistFps_[v] = minDistFps_[u] + 1;
                        customQueue_[tail++] = v;
                    }
                }
            }

            for (int iter = 0; iter < 2; ++iter) {
                std::fill(owner_.begin(), owner_.end(), -1);
                std::fill(curDist_.begin(), curDist_.end(),
                          std::numeric_limits<int>::max());
                std::fill(count_.begin(), count_.end(), 0);
                std::fill(sumRow_.begin(), sumRow_.end(), 0);
                std::fill(sumCol_.begin(), sumCol_.end(), 0);

                head = 0;
                tail = 0;
                for (int i = 0; i < searchSpawnCount_; ++i) {
                    owner_[spawns_[i]] = i;
                    curDist_[spawns_[i]] = 0;
                    customQueue_[tail++] = spawns_[i];
                }

                while (head < tail) {
                    const int u = customQueue_[head++];
                    const int owner = owner_[u];
                    ++count_[owner];
                    sumRow_[owner] += u / width_;
                    sumCol_[owner] += u % width_;
                    const int row = u / width_;
                    const int col = u % width_;
                    for (const auto& dir : kDirections) {
                        const int nr = row + dir[0];
                        const int nc = col + dir[1];
                        if (!inBounds(nr, nc)) continue;
                        const int v = nr * width_ + nc;
                        if (grid_[v] != TILE_BLANK ||
                            curDist_[v] <= curDist_[u] + 1)
                            continue;
                        curDist_[v] = curDist_[u] + 1;
                        owner_[v] = owner;
                        customQueue_[tail++] = v;
                    }
                }

                for (int i = 0; i < searchSpawnCount_; ++i) {
                    if (count_[i] == 0) continue;
                    const long long meanRow = sumRow_[i] / count_[i];
                    const long long meanCol = sumCol_[i] / count_[i];
                    int bestVertex = spawns_[i];
                    long long minDistance =
                        std::numeric_limits<long long>::max();
                    for (int v : maxConnectedComponent_) {
                        if (owner_[v] != i) continue;
                        const long long row = v / width_;
                        const long long col = v % width_;
                        const long long distSq =
                            (row - meanRow) * (row - meanRow) +
                            (col - meanCol) * (col - meanCol);
                        if (distSq >= minDistance) continue;
                        minDistance = distSq;
                        bestVertex = v;
                    }
                    spawns_[i] = bestVertex;
                }
            }

            std::fill(owner_.begin(), owner_.end(), -1);
            std::fill(curDist_.begin(), curDist_.end(),
                      std::numeric_limits<int>::max());
            std::fill(area_.begin(), area_.end(), 0);

            head = 0;
            tail = 0;
            for (int i = 0; i < searchSpawnCount_; ++i) {
                owner_[spawns_[i]] = i;
                curDist_[spawns_[i]] = 0;
                customQueue_[tail++] = spawns_[i];
            }

            int minPairwiseDistance = std::numeric_limits<int>::max();
            while (head < tail) {
                const int u = customQueue_[head++];
                const int owner = owner_[u];
                ++area_[owner];
                const int row = u / width_;
                const int col = u % width_;
                for (const auto& dir : kDirections) {
                    const int nr = row + dir[0];
                    const int nc = col + dir[1];
                    if (!inBounds(nr, nc)) continue;
                    const int v = nr * width_ + nc;
                    if (grid_[v] != TILE_BLANK) continue;

                    if (owner_[v] == -1) {
                        curDist_[v] = curDist_[u] + 1;
                        owner_[v] = owner;
                        customQueue_[tail++] = v;
                    } else if (owner_[v] != owner) {
                        minPairwiseDistance = std::min(
                            minPairwiseDistance, curDist_[u] + curDist_[v] + 1);
                    }
                }
            }

            int maxArea = area_[0];
            int minArea = area_[0];
            for (int i = 1; i < searchSpawnCount_; ++i) {
                maxArea = std::max(maxArea, area_[i]);
                minArea = std::min(minArea, area_[i]);
            }

            int spawnPenalty = 0;
            for (int spawn : spawns_) {
                const int row = spawn / width_;
                const int col = spawn % width_;
                int openNeighbors = 0;
                for (const auto& dir : kDirections) {
                    const int nr = row + dir[0];
                    const int nc = col + dir[1];
                    if (!inBounds(nr, nc)) continue;
                    if (grid_[nr * width_ + nc] == TILE_BLANK) ++openNeighbors;
                }
                if (openNeighbors <= 1)
                    spawnPenalty += 500;
                else if (openNeighbors == 2)
                    spawnPenalty += 50;
            }

            const double averageArea =
                static_cast<double>(maxConnectedComponent_.size()) /
                static_cast<double>(searchSpawnCount_);
            const double areaPenalty =
                static_cast<double>(maxArea - minArea) / averageArea;
            const double componentPenalty =
                static_cast<double>(connectedComponentCount - 1) *
                kComponentPenaltyWeight;
            const double errorPenalty =
                std::abs(currentMountains - targetMountains) * 500.0;
            const double score = areaPenalty * 100.0 - minPairwiseDistance +
                                 spawnPenalty + componentPenalty + errorPenalty;
            if (score >= bestScore) continue;

            bestScore = score;
            bestGrid_ = grid_;
            bestSpawns_ = spawns_;
        }

        grid_ = bestGrid_;
        for (int i = 0;
             i < outputSpawnCount_ && i < static_cast<int>(bestSpawns_.size());
             ++i) {
            grid_[bestSpawns_[i]] = TILE_SPAWN;
        }
    }

    void decorateBorderMountains() {
        std::vector<int> borderMountains;
        borderMountains.reserve(totalTiles_);
        for (int i = 0; i < totalTiles_; ++i) {
            if (grid_[i] != TILE_MOUNTAIN) continue;
            const int row = i / width_;
            const int col = i % width_;
            for (const auto& dir : kDirections) {
                const int nr = row + dir[0];
                const int nc = col + dir[1];
                if (!inBounds(nr, nc)) continue;
                const tile_type_e type = grid_[nr * width_ + nc];
                if (type != TILE_BLANK && type != TILE_SPAWN) continue;
                borderMountains.push_back(i);
                break;
            }
        }

        if (borderMountains.empty()) return;

        const int availableBorderMountains =
            static_cast<int>(borderMountains.size());
        const int lookoutCount =
            placeLookout_ ? uniformPercentageCount(
                                decorateSeed_, availableBorderMountains, 5, 10)
                          : 0;
        const int observatoryCount =
            placeObservatory_
                ? uniformPercentageCount(decorateSeed_,
                                         availableBorderMountains, 5, 10)
                : 0;
        const int cityCount = uniformPercentageCount(
            decorateSeed_, availableBorderMountains, 10, 20);

        for (std::size_t i = 0; i + 1 < borderMountains.size(); ++i) {
            const std::size_t j =
                i + static_cast<std::size_t>(fastRand(decorateSeed_) %
                                             (borderMountains.size() - i));
            std::swap(borderMountains[i], borderMountains[j]);
        }

        std::size_t offset = 0;
        for (int i = 0; i < lookoutCount; ++i)
            grid_[borderMountains[offset++]] = TILE_LOOKOUT;
        for (int i = 0; i < observatoryCount; ++i)
            grid_[borderMountains[offset++]] = TILE_OBSERVATORY;
        for (int i = 0; i < cityCount; ++i)
            grid_[borderMountains[offset++]] = TILE_CITY;
    }

    void placeSwamps() {
        if (!placeSwamp_) return;

        std::vector<int> blankTiles;
        blankTiles.reserve(totalTiles_);
        for (int i = 0; i < totalTiles_; ++i)
            if (grid_[i] == TILE_BLANK) blankTiles.push_back(i);

        const int swampCount = uniformPercentageCount(
            decorateSeed_, static_cast<int>(blankTiles.size()), 8, 15);
        if (swampCount <= 0) return;

        for (int i = 0; i < swampCount; ++i) {
            const int pick = i + static_cast<int>(fastRand(decorateSeed_) %
                                                  (blankTiles.size() - i));
            std::swap(blankTiles[i], blankTiles[pick]);
            grid_[blankTiles[i]] = TILE_SWAMP;
        }
    }

    Board buildBoard() {
        Board board(height_, width_);
        for (int row = 0; row < height_; ++row) {
            for (int col = 0; col < width_; ++col) {
                Tile& tile = board.tileAt(row + 1, col + 1);
                const tile_type_e type = grid_[row * width_ + col];
                tile = Tile(-1, type, 0);
                if (type == TILE_SPAWN) {
                    tile.spawnTeam = 0;
                } else if (type == TILE_CITY) {
                    tile.army = uniformInclusive(cityArmySeed_, 40, 49);
                }
            }
        }
        return board;
    }

    int width_;
    int height_;
    int totalTiles_;
    int outputSpawnCount_;
    int searchSpawnCount_;
    bool placeSwamp_;
    bool placeLookout_;
    bool placeObservatory_;
    double mountainDensity_;
    std::uint32_t searchSeed_;
    std::uint32_t decorateSeed_;
    std::uint32_t cityArmySeed_;

    std::vector<tile_type_e> grid_;
    std::vector<tile_type_e> bestGrid_;
    std::vector<std::uint8_t> visited_;
    std::vector<std::uint8_t> isCandidate_;
    std::vector<int> customQueue_;
    std::vector<int> maxConnectedComponent_;
    std::vector<int> minDistFps_;
    std::vector<int> curDist_;
    std::vector<int> owner_;
    std::vector<int> area_;
    std::vector<long long> sumRow_;
    std::vector<long long> sumCol_;
    std::vector<long long> count_;
    std::vector<int> spawns_;
    std::vector<int> bestSpawns_;
    std::vector<int> erosionCandidates_;
};

inline Board generateBoard(int width, int height, int spawnCount,
                           std::mt19937::result_type seed, bool placeSwamp,
                           bool placeLookout, bool placeObservatory,
                           double mountainDensity) {
    return Generator(width, height, spawnCount, seed, placeSwamp, placeLookout,
                     placeObservatory, mountainDensity)
        .run();
}

}  // namespace

inline Board Board::generate(int width, int height, int spawnCount,
                             std::mt19937::result_type seed, bool placeSwamp,
                             bool placeLookout, bool placeObservatory,
                             double mountainDensity) {
    return generateBoard(width, height, spawnCount, seed, placeSwamp,
                         placeLookout, placeObservatory, mountainDensity);
}

#endif  // LGEN_CORE_MAPGEN_HPP
