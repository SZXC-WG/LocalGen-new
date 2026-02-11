/**
 * @file utils.h
 *
 * LocalGen Module: GameEngine
 *
 * Utility functions and definitions
 */

#ifndef LGEN_GAMEENGINE_UTILS_H
#define LGEN_GAMEENGINE_UTILS_H

#include <cstdint>

// Common type aliases

using index_t = int32_t;
using army_t = int64_t;
using pos_t = int32_t;
using turn_t = uint32_t;
using speed_t = double;

// Coordinate definition & operations

struct Coord {
    pos_t x, y;
    constexpr Coord() : x(0), y(0) {}
    constexpr Coord(pos_t _x, pos_t _y) : x(_x), y(_y) {}
    inline bool operator==(const Coord& other) const {
        return x == other.x && y == other.y;
    }
    inline bool operator!=(const Coord& other) const {
        return x != other.x || y != other.y;
    }
    inline bool operator<(const Coord& other) const {
        return x == other.x ? y < other.y : x < other.x;
    }
};

#endif  // LGEN_GAMEENGINE_UTILS_H