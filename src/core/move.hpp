// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file move.hpp
 *
 * LocalGen Module: core
 *
 * Moves
 *
 * Moves are basic operations of players.
 */

#include "utils.hpp"

/// Move types.
/// Set value for priority settings.
enum class MoveType {
    EMPTY = 0,
    SURRENDER = 1,
    MOVE_ARMY = 2,
};

/// Stores a single move: `from` -> `to`.
struct Move {
    MoveType type = MoveType::EMPTY;
    Coord from, to;
    bool takeHalf = false;

    constexpr Move() = default;
    constexpr Move(MoveType type) : type(type) {}
    constexpr Move(MoveType type, Coord from, Coord to, bool takeHalf)
        : type(type), from(from), to(to), takeHalf(takeHalf) {}
};
