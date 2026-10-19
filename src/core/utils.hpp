// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file utils.hpp
 *
 * LocalGen Module: core
 *
 * Utility functions and definitions
 */

#include <bit>
#include <compare>
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
    auto operator<=>(const Coord&) const = default;
    constexpr Coord operator+(Coord r) const noexcept {
        return {x + r.x, y + r.y};
    }
};

template <>
struct std::hash<Coord> {
    constexpr size_t operator()(Coord c) const noexcept {
        return std::bit_cast<size_t>(c);
    }
};

/// Helper for std::visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
