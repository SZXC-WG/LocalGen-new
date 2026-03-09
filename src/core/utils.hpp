/**
 * @file utils.hpp
 *
 * LocalGen Module: core
 *
 * Utility functions and definitions
 */

#ifndef LGEN_CORE_UTILS_HPP
#define LGEN_CORE_UTILS_HPP

#include <cstdint>
#include <functional>

// Common type aliases

using index_t = int32_t;
using army_t = int64_t;
using pos_t = int32_t;
using turn_t = uint32_t;
using speed_t = double;

// Coordinate definition & operations

struct Coord {
    pos_t x = 0, y = 0;
    constexpr Coord() = default;
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

inline Coord operator+(const Coord& lhs, const Coord& rhs) {
    return Coord(lhs.x + rhs.x, lhs.y + rhs.y);
}

/// Hash function for Coord, enables use in std::unordered_map.
struct CoordHash {
    inline std::size_t operator()(const Coord& c) const {
        return std::hash<pos_t>{}(c.x) ^ (std::hash<pos_t>{}(c.y) << 1);
    }
};

/// Helper for std::visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif  // LGEN_CORE_UTILS_HPP