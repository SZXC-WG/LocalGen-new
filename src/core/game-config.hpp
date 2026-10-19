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

#include <concepts>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

enum class VisionMode : uint8_t { INHERIT, NEAR8, NEAR4 };
enum class MoveOrderMode : uint8_t { PRIORITY, ALTERNATING_INDEX };

struct GameConfig {
    // ---- Display settings ----
    bool RanklistShowLand = true;
    bool RanklistShowArmy = true;
    bool RanklistShowPlayerName = true;
    bool RanklistShowColor = true;

    // ---- Vision settings ----
    VisionMode OverallVisionMode = VisionMode::NEAR8;
    int OverallVisionRange = 1;
    VisionMode CityVisionMode = VisionMode::INHERIT;
    int CityVisionRange = -1;  // -1 means to inherit from overall

    // ---- Move settings ----
    MoveOrderMode MoveOrder = MoveOrderMode::PRIORITY;

    // ---- Modifier flags ----
    bool CrystalClearEnabled = false;
    bool MistyVeilEnabled = false;
    bool LeapfrogEnabled = false;
    bool CityStateEnabled = false;
    bool DefenselessEnabled = false;
    bool DefectionEnabled = false;
    bool SlipperyEnabled = false;
    int FadingSmogInterval = 0;  // 0 means disabled

    constexpr bool operator==(const GameConfig&) const noexcept = default;
};

inline constexpr GameConfig defaultConf{};

namespace modifiers {

// ---- Vision modifiers ----
inline constexpr GameConfig Watchtower{
    .CityVisionMode = VisionMode::NEAR4,
    .CityVisionRange = 4,
};
inline constexpr GameConfig MistyVeil{
    .OverallVisionRange = 0,
    .MistyVeilEnabled = true,
};
inline constexpr GameConfig CrystalClear{.CrystalClearEnabled = true};
inline constexpr GameConfig FadingSmog{.FadingSmogInterval = 25};

// ---- Behavioral modifiers ----
inline constexpr GameConfig Leapfrog{.LeapfrogEnabled = true};
inline constexpr GameConfig CityState{.CityStateEnabled = true};
inline constexpr GameConfig Defenseless{.DefenselessEnabled = true};
inline constexpr GameConfig Defection{.DefectionEnabled = true};
inline constexpr GameConfig Slippery{.SlipperyEnabled = true};

// ---- Display modifiers ----
inline constexpr GameConfig SilentWar{
    .RanklistShowLand = false,
    .RanklistShowArmy = false,
};

}  // namespace modifiers

constexpr bool isValidConfig(const GameConfig& conf) noexcept {
    return !(conf.CrystalClearEnabled && conf.MistyVeilEnabled);
}

namespace detail {

template <typename T>
concept GameConfigType = std::same_as<std::remove_cv_t<T>, GameConfig>;

constexpr auto asTuple(GameConfigType auto& c) noexcept {
    auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
           f16] = c;
    return std::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13,
                    f14, f15, f16);
}

template <std::size_t I = 0>
constexpr void forEachField(auto&& fn,
                            GameConfigType auto&... configs) noexcept {
    if constexpr (I < std::tuple_size_v<decltype(asTuple(defaultConf))>) {
        static_assert(
            noexcept(fn(std::get<I>(asTuple(configs))...)),
            "forEachField requires a noexcept callback for every field");
        fn(std::get<I>(asTuple(configs))...);
        forEachField<I + 1>(fn, configs...);
    }
}

}  // namespace detail

constexpr GameConfig& operator|=(GameConfig& lhs,
                                 const GameConfig& rhs) noexcept {
    detail::forEachField(
        [](auto& l, const auto& r, const auto& d) noexcept {
            if (r != d) l = r;
        },
        lhs, rhs, defaultConf);
    return lhs;
}

constexpr GameConfig operator|(GameConfig lhs, const GameConfig& rhs) noexcept {
    return lhs |= rhs;
}

enum class PatchStatus : uint8_t {
    FULLY_ENABLED,
    PARTIALLY_ENABLED,
    DISABLED,
    OVERRIDDEN
};

constexpr PatchStatus patchStatus(const GameConfig& config,
                                  const GameConfig& preset) noexcept {
    bool allPreset = true, anyPreset = false, allDefault = true;
    detail::forEachField(
        [&](const auto& c, const auto& p, const auto& d) noexcept {
            if (p != d) {
                if (c == p)
                    anyPreset = true;
                else
                    allPreset = false;
                if (c != d) allDefault = false;
            }
        },
        config, preset, defaultConf);
    if (allPreset) return PatchStatus::FULLY_ENABLED;
    if (allDefault) return PatchStatus::DISABLED;
    if (anyPreset) return PatchStatus::PARTIALLY_ENABLED;
    return PatchStatus::OVERRIDDEN;
}
