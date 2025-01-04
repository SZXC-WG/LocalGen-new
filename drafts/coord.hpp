/**
 * @file coord.hpp
 * 
 * LocalGen Library: coordinates
 * 
 * Coordinates play an important role in LocalGen. They are used in various
 * situations, (e.g. tile positions). LocalGen therefore specifies
 * a common coordinate system that is used throughout the project.
 */

#ifndef LGEN_LIB_COORD
#define LGEN_LIB_COORD 1

#include <cstdint>

using pos_t = int32_t;

/// Struct for Coordinates, %Coord.
/// Avoid using `COORD` as windows.h defines it.
struct Coord {
    pos_t x, y;
    Coord() :
        x(0), y(0) {}
    Coord(pos_t _x, pos_t _y) :
        x(_x), y(_y) {}
};

// Sometimes we need to pass a %Coord through two parameters or something.
// This macro is used to make it available.
// windows.h `COORD` have uppercase members X and Y, so it's not compatible.
#define SEPA(coo) (coo).x, (coo).y

/// Comparison operators for %Coord.
/// `operator==` and (before C++2a) `operator!=` are defined as usual.
/// `operator<` series acts like std::pair of convenience for sorting and containers like std::set and std::map.

bool operator==(const Coord& a, const Coord& b) { return (a.x == b.x && a.y == b.y); }
#if __cplusplus < 202002L
bool operator!=(const Coord& a, const Coord& b) { return !(a == b); }
#endif  // before C++2a
bool operator<(const Coord& a, const Coord& b) { return (a.x < b.x || (a.x == b.x && a.y < b.y)); }
bool operator>(const Coord& a, const Coord& b) { return b < a; }
bool operator<=(const Coord& a, const Coord& b) { return !(b < a); }
bool operator>=(const Coord& a, const Coord& b) { return !(a < b); }

/// Calculation operators for %Coord.
/// Only `operator+` and `operator-` for coordinates can only do this.

Coord operator+(const Coord& a, const Coord& b) { return Coord(a.x + b.x, a.y + b.y); }
Coord operator-(const Coord& a, const Coord& b) { return Coord(a.x - b.x, a.y - b.y); }

#endif  // LGEN_LIB_COORD
