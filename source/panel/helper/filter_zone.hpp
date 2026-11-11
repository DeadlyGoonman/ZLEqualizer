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

    // fraction (in dB) of the full shelf gain that must be reached under the cursor
    inline constexpr double kShelfPlateauFraction = .95;
    // below this magnitude the plateau distance is evaluated with this gain, the ratio hardly changes there
    inline constexpr double kShelfMinGainDb = .5;

    /**
     * magnitude in dB of the second order shelf of the Ideal prototype at w / w0 = x, w0 = 1
     * (same coefficients as zldsp::filter::IdealCoeff::get2LowShelf / get2HighShelf)
     * @param gain_db the full (plateau) gain of the shelf
     * @param q the q of the shelf section
     */
    inline double getShelfMagnitudeDb(const double gain_db, const double q, const bool is_high, const double x) {
        const auto a = std::pow(10.0, gain_db / 40.0); // sqrt of the amplitude gain
        const auto awq = std::sqrt(a) / q;
        const auto w2 = x * x;
        double num, den;
        if (is_high) {
            // {Awq, A, A^2, A*Awq, A}
            num = (a * awq) * (a * awq) * w2 + (a - a * a * w2) * (a - a * a * w2);
            den = awq * awq * w2 + (a - w2) * (a - w2);
        } else {
            // {Awq/A, 1/A, 1, Awq, A}
            num = awq * awq * w2 + (a - w2) * (a - w2);
            den = (awq / a) * (awq / a) * w2 + (1.0 / a - w2) * (1.0 / a - w2);
        }
        return 10.0 * std::log10(num / den);
    }

    /**
     * how far the shelf corner is placed from the cursor, so that the shelf curve has reached
     * kShelfPlateauFraction of its full gain under the cursor
     * low shelf: corner = cursor * ratio, high shelf: corner = cursor / ratio
     * @param gain_db the full (plateau) gain of the shelf
     * @param q the user q of the band (the shelf section uses sqrt(q * sqrt(2)) / sqrt(2))
     * @return the ratio, at least 1
     */
    inline double getShelfCornerRatio(const double gain_db, const double q, const bool is_high) {
        const auto g = std::abs(gain_db) < kShelfMinGainDb ? (gain_db < 0.0 ? -kShelfMinGainDb : kShelfMinGainDb)
                                                           : gain_db;
        const auto q_shelf = std::sqrt(q * std::sqrt(2.0)) / std::sqrt(2.0);
        // fraction of the plateau reached at distance ratio from the corner, grows with the ratio
        const auto fraction = [&](const double ratio) {
            const auto x = is_high ? ratio : 1.0 / ratio;
            return getShelfMagnitudeDb(g, q_shelf, is_high, x) / g;
        };
        double lo = 1.0, hi = 64.0;
        if (fraction(hi) < kShelfPlateauFraction) {
            return hi;
        }
        for (int i = 0; i < 40; ++i) {
            const auto mid = std::sqrt(lo * hi);
            if (fraction(mid) < kShelfPlateauFraction) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return hi;
    }
}
