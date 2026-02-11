/**
 * @file coord.h
 *
 * LocalGen Library: coordinates
 *
 * Coordinates play an important role in LocalGen. They are used in various
 * situations, (e.g. tile positions). LocalGen therefore specifies
 * a common coordinate system that is used throughout the project.
 */

#ifndef LGEN_LIB_COORD_H
#define LGEN_LIB_COORD_H

#include <cstdint>
#include <utility>

using pos_t = int32_t;

/// Struct for Coordinates, %Coord.
struct Coord {
    pos_t x, y;
    constexpr Coord() : x(0), y(0) {}
    constexpr Coord(pos_t _x, pos_t _y) : x(_x), y(_y) {}

    static const pos_t COORD_INDEX = 200;
    /// Convert %Coord to a single index.
    inline pos_t index() const;
};

/// Link two %Coords together to a single "biindex".
inline pos_t biindex(const Coord& a, const Coord& b);
/// Unpacking a "biindex" to two %Coords.
inline std::pair<Coord, Coord> unpack_biindex(const pos_t& bi);

// Sometimes we need to pass a %Coord through two parameters or something.
// This macro is used to make it easier.
#define SEPA(coo) (coo).x, (coo).y

/// Comparison operators for %Coord.
/// `operator==` and `operator!=` are defined as usual.
/// `operator<` series acts like std::pair of convenience for sorting and
/// comparison-relying containers like std::set and std::map.

bool operator==(const Coord& a, const Coord& b);
#if __cplusplus < 202002L
bool operator!=(const Coord& a, const Coord& b);
#endif  // before C++2a
bool operator<(const Coord& a, const Coord& b);
bool operator>(const Coord& a, const Coord& b);
bool operator<=(const Coord& a, const Coord& b);
bool operator>=(const Coord& a, const Coord& b);

/// Calculation operators for %Coord.
/// Only `operator+` and `operator-` since coordinates can only do this.

Coord operator+(const Coord& a, const Coord& b);
Coord operator-(const Coord& a, const Coord& b);

#endif  // LGEN_LIB_COORD_H
