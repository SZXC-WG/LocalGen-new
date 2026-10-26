// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file game-config.hpp
 *
 * LocalGen Module: core
 *
 * Game Configurations
 *
 * Each modifier is an independent boolean, disabled by default.
 */

struct GameConfig {
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

constexpr bool isValidConfig(const GameConfig& conf) noexcept {
    return !(conf.crystalClear && conf.mistyVeil);
}
