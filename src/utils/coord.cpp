/**
 * @file coord.hpp
 *
 * LocalGen Library: coordinates
 *
 * Coordinates play an important role in LocalGen. They are used in various
 * situations, (e.g. tile positions). LocalGen therefore specifies
 * a common coordinate system that is used throughout the project.
 */

#ifndef LGEN_LIB_COORD_CPP
#define LGEN_LIB_COORD_CPP 1

#include "coord.h"

Coord::Coord() : x(0), y(0) {}
Coord::Coord(pos_t _x, pos_t _y) : x(_x), y(_y) {}

inline pos_t Coord::index() const { return x * COORD_INDEX + y; }

inline pos_t biindex(const Coord& a, const Coord& b) {
    return a.index() * Coord::COORD_INDEX * Coord::COORD_INDEX + b.index();
}
inline std::pair<Coord, Coord> unpack_biindex(const pos_t& bi) {
    pos_t fi = bi / (Coord::COORD_INDEX * Coord::COORD_INDEX);
    pos_t se = bi % (Coord::COORD_INDEX * Coord::COORD_INDEX);
    return std::pair(Coord(fi / Coord::COORD_INDEX, fi % Coord::COORD_INDEX),
                     Coord(se / Coord::COORD_INDEX, se % Coord::COORD_INDEX));
}

bool operator==(const Coord& a, const Coord& b) {
    return (a.x == b.x && a.y == b.y);
}
#if __cplusplus < 202002L
bool operator!=(const Coord& a, const Coord& b) { return !(a == b); }
#endif  // before C++2a
bool operator<(const Coord& a, const Coord& b) {
    return (a.x < b.x || (a.x == b.x && a.y < b.y));
}
bool operator>(const Coord& a, const Coord& b) { return b < a; }
bool operator<=(const Coord& a, const Coord& b) { return !(b < a); }
bool operator>=(const Coord& a, const Coord& b) { return !(a < b); }

Coord operator+(const Coord& a, const Coord& b) {
    return Coord(a.x + b.x, a.y + b.y);
}
Coord operator-(const Coord& a, const Coord& b) {
    return Coord(a.x - b.x, a.y - b.y);
}

#endif  // LGEN_LIB_COORD_CPP
