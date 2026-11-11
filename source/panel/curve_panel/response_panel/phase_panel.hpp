// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../state/ui_state.hpp"

#include "../../../PluginProcessor.hpp"
#include "../../../gui/gui.hpp"
#include "../../helper/helper.hpp"

namespace zlpanel {
    /**
     * the optional phase response line of the whole EQ, +-180 degrees span the graph height
     * the path is built on the response thread and only drawn on the message thread
     */
    class PhasePanel final : public juce::Component {
    public:
        explicit PhasePanel(zlgui::UIState& base);

        /**
         * draw the reference line, the labels and the phase line, does nothing while the overlay is off
         */
        void paintPhase(juce::Graphics& g);

        /**
         * response thread: build the phase path
         * @param xs x positions of the points
         * @param phases continuous (unwrapped) phase in rad
         * @param zero_y y position of the 0 degree line (the 0 dB line)
         * @param half_height the distance in pixels from 0 to +-180 degrees
         */
        void run(std::span<const float> xs, std::span<const float> phases, float zero_y, float half_height);

        /**
         * response thread: drop the path (nothing to draw)
         */
        void clear();

    private:
        struct Data {
            juce::Path path;
            float zero_y{0.f};
            float half_height{0.f};
            bool valid{false};
        };

        zlgui::UIState& base_;
        TriBuffer<Data> data_{};
    };
}
