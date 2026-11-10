// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file modifiers.hpp
 *
 * LocalGen Module: core
 *
 * Game Modifiers
 *
 * Each modifier is an independent boolean, disabled by default.
 */

struct Modifiers {
    // Vision modifiers
    bool watchtower = false;
    bool mistyVeil = false;
    bool crystalClear = false;
    bool fadingSmog = false;

    // Behavioral modifiers
    bool leapfrog = false;
    bool cityState = false;
    bool defenseless = false;
    bool defection = false;
    bool slippery = false;

    // Display modifiers
    bool silentWar = false;
};

constexpr bool isValidModifiers(const Modifiers& mod) noexcept {
    return !(mod.crystalClear &&
             (mod.watchtower || mod.mistyVeil || mod.fadingSmog));
}
