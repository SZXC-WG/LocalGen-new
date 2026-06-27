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
 * Configuration base for games
 */

#include <cstdint>
#include <optional>

namespace config {

enum class VisionMode : uint8_t { NEAR8, NEAR4 };
enum class MoveProcessMode : uint8_t { FULL, PARITY };

#define GAME_CONFIG_UNIT_LIST(F)                                 \
    /* ---- Display settings ---- */                             \
    F(bool, RanklistShowLand, true)                              \
    F(bool, RanklistShowArmy, true)                              \
    F(bool, RanklistShowPlayerName, true)                        \
    F(bool, RanklistShowColor, true)                             \
    /* ---- Vision settings ---- */                              \
    F(VisionMode, OverallVisionMode, VisionMode::NEAR8)          \
    F(int, OverallVisionRange, 1)                                \
    F(int, CityVisionRange, 1)                                   \
    F(int, SpawnVisionRange, 1)                                  \
    /* ---- Move settings ---- */                                \
    F(MoveProcessMode, MoveProcessMethod, MoveProcessMode::FULL) \
    /* ---- Modifier flags ---- */                               \
    F(bool, MistyVeilEnabled, false)                             \
    F(bool, LeapfrogEnabled, false)                              \
    F(bool, CityStateEnabled, false)                             \
    F(bool, DefenselessEnabled, false)                           \
    F(bool, DefectionEnabled, false)                             \
    F(bool, SlipperyEnabled, false)                              \
    F(bool, FadingSmogEnabled, false)                            \
    F(int, FadingSmogInterval, 25)

struct Config {
#define DECL(type, name, def) type name = def;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

#define IF_EQUAL(type, name, ...) \
    if (lhs.name != rhs.name) return false;
constexpr inline bool operator==(const Config& lhs, const Config& rhs) {
    GAME_CONFIG_UNIT_LIST(IF_EQUAL)
    return true;
}
#undef IF_EQUAL

struct ConfigPatch {
#define DECL(type, name, ...) std::optional<type> name;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

#define IF_EQUAL(type, name, ...) \
    if (lhs.name != rhs.name) return false;
constexpr inline bool operator==(const ConfigPatch& lhs,
                                 const ConfigPatch& rhs) {
    GAME_CONFIG_UNIT_LIST(IF_EQUAL)
    return true;
}
#undef IF_EQUAL

namespace unit {
#define UNIT(type, name, ...)            \
    constexpr ConfigPatch name(type v) { \
        ConfigPatch p;                   \
        p.name = v;                      \
        return p;                        \
    }
GAME_CONFIG_UNIT_LIST(UNIT)
#undef UNIT
}  // namespace unit

#define IF_ASSIGN(type, name, ...) \
    if (rhs.name) res.name = rhs.name;
constexpr inline ConfigPatch operator|(const ConfigPatch& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN)
    return res;
}
#undef IF_ASSIGN

#define IF_ASSIGN_VALUE(type, name, ...) \
    if (rhs.name) res.name = *rhs.name;
constexpr inline Config operator|(const Config& lhs, const ConfigPatch& rhs) {
    Config res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_VALUE)
    return res;
}
constexpr inline Config operator|(const ConfigPatch& lhs, const Config& rhs) {
    return rhs | lhs;
}
#undef IF_ASSIGN_VALUE

#define IF_ASSIGN_OPTIONAL(type, name, ...) \
    if (rhs.name) res.name = lhs.name;
constexpr inline ConfigPatch operator&(const Config& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_OPTIONAL);
    return res;
}
constexpr inline ConfigPatch operator&(const ConfigPatch& lhs,
                                       const Config& rhs) {
    return rhs & lhs;
}
#undef IF_ASSIGN_OPTIONAL

#define GAME_CONFIG_MODIFIER_LIST(F)                                         \
    /* ---- Vision modifiers ---- */                                         \
    F(Watchtower, unit::CityVisionRange(5) | unit::SpawnVisionRange(5))      \
    F(MistyVeil, unit::MistyVeilEnabled(true) | unit::OverallVisionRange(0)) \
    F(CrystalClear, unit::OverallVisionRange(100))                           \
    F(FadingSmog,                                                            \
      unit::FadingSmogEnabled(true) | unit::FadingSmogInterval(25))          \
    /* ---- Behavioral modifiers ---- */                                     \
    F(Leapfrog, unit::LeapfrogEnabled(true))                                 \
    F(CityState, unit::CityStateEnabled(true))                               \
    F(Defenseless, unit::DefenselessEnabled(true))                           \
    F(Defection, unit::DefectionEnabled(true))                               \
    F(Slippery, unit::SlipperyEnabled(true))                                 \
    /* ---- Display modifiers ---- */                                        \
    F(SilentWar, unit::RanklistShowLand(false) | unit::RanklistShowArmy(false))

namespace modifier {
#define MODIFIER(name, value) constexpr ConfigPatch name = value;
GAME_CONFIG_MODIFIER_LIST(MODIFIER)
#undef MODIFIER
}  // namespace modifier

constexpr Config defaultConf;

enum class PatchStatus : uint8_t {
    FULLY_ENABLED,
    PARTIALLY_ENABLED,
    DISABLED,
    OVERRIDDEN
};

constexpr inline PatchStatus patchStatus(const Config& config,
                                         const ConfigPatch& patch) {
    ConfigPatch confPatch = config & patch;
    ConfigPatch defPatch = defaultConf & patch;
    if (confPatch == patch) return PatchStatus::FULLY_ENABLED;
    if (confPatch == defPatch) return PatchStatus::DISABLED;
#define IF_MIXED(type, name, ...) \
    if (confPatch.name == patch.name) return PatchStatus::PARTIALLY_ENABLED;
    GAME_CONFIG_UNIT_LIST(IF_MIXED)
#undef IF_MIXED
    return PatchStatus::OVERRIDDEN;
}

#undef GAME_CONFIG_UNIT_LIST
#undef GAME_CONFIG_MODIFIER_LIST

}  // namespace config
