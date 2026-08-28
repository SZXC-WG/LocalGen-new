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
    std::string suffix;

    Version() = default;
    constexpr Version(count_t major, count_t minor, count_t patch,
                      BuildType type = BuildType::Dev, count_t count = 0)
        : major(major),
          minor(minor),
          patch(patch),
          suffix(buildStr(type, count)) {}

    std::string str() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." +
               std::to_string(patch) + suffix;
    }
};

const Version version{4, 3, 4, BuildType::Dev};

}  // namespace version_namespace

using version_namespace::version;
