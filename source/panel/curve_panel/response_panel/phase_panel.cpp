// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "phase_panel.hpp"

#include <cmath>
#include <numbers>

namespace zlpanel {
    PhasePanel::PhasePanel(zlgui::UIState& base) : base_(base) {
        setInterceptsMouseClicks(false, false);
    }

    void PhasePanel::paintPhase(juce::Graphics& g) {
        if (!base_.getPhaseCurveEnabled()) {
            return;
        }
        data_.pull();
        const auto& data{data_.getReader()};
        if (!data.valid) {
            return;
        }
        const auto font_size = base_.gui.getFontSize();
        const auto colour = base_.getPhaseCurveColour();
        const auto width = static_cast<float>(getWidth());
        // faint 0 degree reference on the 0 dB line
        g.setColour(colour.withMultipliedAlpha(.25f));
        g.drawHorizontalLine(juce::roundToInt(data.zero_y), 0.f, width);
        // labels on the left edge
        g.setColour(colour.withMultipliedAlpha(.7f));
        g.setFont(font_size * .8f);
        const auto label_height = font_size * 1.1f;
        constexpr std::array<int, 4> kLabelDegrees{180, 90, -90, -180};
        for (const auto degree : kLabelDegrees) {
            const auto y = data.zero_y - static_cast<float>(degree) / 180.f * data.half_height;
            const auto text = juce::String(degree > 0 ? "+" : "") + juce::String(degree) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
            g.drawText(text,
                       juce::Rectangle<float>(font_size * .3f, y - label_height * .5f, font_size * 4.f, label_height),
                       juce::Justification::centredLeft, false);
        }
        // the phase line
        g.setColour(colour);
        g.strokePath(data.path, juce::PathStrokeType(font_size * .12f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::butt));
    }

    void PhasePanel::run(const std::span<const float> xs, const std::span<const float> phases,
                         const float zero_y, const float half_height) {
        auto& data{data_.getWriter()};
        data.path.clear();
        data.zero_y = zero_y;
        data.half_height = half_height;
        data.valid = false;
        if (xs.size() < 2 || xs.size() != phases.size()) {
            data_.publish();
            return;
        }
        constexpr auto kPi = std::numbers::pi_v<float>;
        constexpr auto kTwoPi = 2.f * kPi;
        // fold the continuous phase back to +-180 degrees, the line leaves at one edge and enters at the other
        const auto getY = [&](const float folded) { return zero_y - folded / kPi * half_height; };
        auto previous_turn = std::round(phases[0] / kTwoPi);
        data.path.startNewSubPath(xs[0], getY(phases[0] - previous_turn * kTwoPi));
        for (size_t i = 1; i < xs.size(); ++i) {
            const auto turn = std::round(phases[i] / kTwoPi);
            if (turn != previous_turn) {
                const auto rising = turn > previous_turn;
                const auto level = (previous_turn * 2.f + (rising ? 1.f : -1.f)) * kPi;
                const auto t = std::clamp((level - phases[i - 1]) / (phases[i] - phases[i - 1]), 0.f, 1.f);
                const auto cross_x = xs[i - 1] + t * (xs[i] - xs[i - 1]);
                data.path.lineTo(cross_x, getY(rising ? kPi : -kPi));
                data.path.startNewSubPath(cross_x, getY(rising ? -kPi : kPi));
                previous_turn = turn;
            }
            data.path.lineTo(xs[i], getY(phases[i] - turn * kTwoPi));
        }
        data.valid = true;
        data_.publish();
    }

    void PhasePanel::clear() {
        auto& data{data_.getWriter()};
        data.path.clear();
        data.valid = false;
        data_.publish();
    }
}
