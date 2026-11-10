// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <cmath>

namespace zlpanel::band_helper {
    enum class Zone { kCut, kShelf, kPeak };

    // the low side is the mirror of the high side on the log axis: f_low = kMirrorProduct / f_high
    inline constexpr float kMirrorProduct = 400000.f; // 20 Hz * 20 kHz
    inline constexpr float kCutEdge = 17000.f; // mirrored: ~23.5 Hz
    inline constexpr float kCutFull = 20000.f; // mirrored: 20 Hz
    inline constexpr float kShelfEdge = 6250.f; // mirrored: 64 Hz
    inline constexpr float kCutNearZero = .2f; // |gain portion| below which a cut is created at kCutFull

    /**
     * @param is_low whether the frequency is on the low side of the (log) centre of the axis
     */
    inline Zone getInitialZone(const float frequency, const float gain_portion, bool& is_low) {
        is_low = frequency * frequency < kMirrorProduct;
        const auto mirrored = is_low ? kMirrorProduct / std::max(frequency, 1.f) : frequency;
        if (mirrored > kCutEdge) {
            // the near-zero window grows smoothly from nothing at the cut edge to full at the axis end
            const auto t = std::clamp(std::log(mirrored / kCutEdge) / std::log(kCutFull / kCutEdge), 0.f, 1.f);
            return std::abs(gain_portion) < kCutNearZero * t ? Zone::kCut : Zone::kShelf;
        }
        return mirrored > kShelfEdge ? Zone::kShelf : Zone::kPeak;
    }
}
