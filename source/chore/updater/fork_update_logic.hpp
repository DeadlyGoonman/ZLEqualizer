// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <optional>

#include <juce_core/juce_core.h>

#ifndef ZL_FORK_BUILD
#define ZL_FORK_BUILD 0
#endif

namespace zlupdate {
    inline constexpr int kForkBuild = ZL_FORK_BUILD;

    inline const char* const kRepo = "DeadlyGoonman/ZLEqualizer";
    inline const char* const kBundleName = "ZL Equalizer 2.vst3";
    inline const char* const kBundleBinary = "ZL Equalizer 2.vst3/Contents/x86_64-win/ZL Equalizer 2.vst3";

    struct ReleaseInfo {
        int build{0};
        juce::String asset_url;
        juce::String page_url;
    };

    // "fork-12" -> 12, anything else -> -1
    inline int parseForkTag(const juce::String& tag) {
        if (!tag.startsWith("fork-")) {
            return -1;
        }
        const auto digits = tag.substring(5);
        if (digits.isEmpty() || digits.length() > 6 || !digits.containsOnly("0123456789")) {
            return -1;
        }
        return digits.getIntValue();
    }

    inline juce::String getAssetName(const int build) {
        return "ZLEqualizer-fork-" + juce::String(build) + "-windows-vst3.zip";
    }

    inline juce::String getPageURL(const int build) {
        return juce::String("https://github.com/") + kRepo + "/releases/tag/fork-" + juce::String(build);
    }

    // direct download link GitHub serves for a release asset
    inline juce::String getDefaultAssetURL(const int build) {
        return juce::String("https://github.com/") + kRepo + "/releases/download/fork-" + juce::String(build)
            + "/" + getAssetName(build);
    }

    // Parses a GitHub "latest release" response. Returns a release only if it is newer than
    // `current_build` and carries the expected zip asset. Malformed input yields nullopt.
    inline std::optional<ReleaseInfo> findNewerRelease(const juce::String& json_text, const int current_build) {
        const auto json = juce::JSON::parse(json_text);
        if (!json.isObject()) {
            return std::nullopt;
        }
        const auto build = parseForkTag(json.getProperty("tag_name", {}).toString());
        if (build < 0 || build <= current_build) {
            return std::nullopt;
        }
        const auto* assets = json.getProperty("assets", {}).getArray();
        if (assets == nullptr) {
            return std::nullopt;
        }
        const auto expected = getAssetName(build);
        for (const auto& asset : *assets) {
            if (asset.getProperty("name", {}).toString() != expected) {
                continue;
            }
            const auto url = asset.getProperty("browser_download_url", {}).toString();
            if (!url.startsWith("https://")) {
                continue;
            }
            ReleaseInfo info;
            info.build = build;
            info.asset_url = url;
            info.page_url = getPageURL(build);
            return info;
        }
        return std::nullopt;
    }
}
