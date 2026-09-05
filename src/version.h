// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: MIT

/**
 * version.h
 *
 * New version control of LG v5
 */

#pragma once

#include <cstdint>
#include <string>

namespace version_namespace {

enum class BuildType : std::uint8_t { Rel, RC, Beta, Alpha, Dev };

typedef std::uint16_t count_t;

#define VER_MAJOR 4
#define VER_MINOR 3
#define VER_PATCH 4
#define VER_TYPE BuildType::Dev
#define VER_COUNT

inline std::string buildStr(BuildType type = BuildType::Dev,
                            count_t count = 0) {
    switch (type) {
        case BuildType::Rel:   return "";
        case BuildType::RC:    return "-rc." + std::to_string(count);
        case BuildType::Beta:  return "-beta." + std::to_string(count);
        case BuildType::Alpha: return "-alpha." + std::to_string(count);
        case BuildType::Dev:   return "-dev";
        default:               return "-unknown";
    }
}

struct Version {
    count_t major, minor, patch;
    BuildType type;
    count_t count;

    Version() = default;
    constexpr Version(count_t major, count_t minor, count_t patch,
                      BuildType type = BuildType::Dev, count_t count = 0)
        : major(major), minor(minor), patch(patch), type(type), count(count) {}

    std::string str() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." +
               std::to_string(patch) + buildStr(type, count);
    }
};

constexpr Version version{VER_MAJOR, VER_MINOR, VER_PATCH, VER_TYPE, VER_COUNT};

}  // namespace version_namespace

using version_namespace::version;

#define VER_STRING version.str().c_str()
