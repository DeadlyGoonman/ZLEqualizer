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
#include "../../../chore/thread/notifier.hpp"

namespace zlpanel {
    /**
     * a faint preview of the band that would be created at the mouse position
     */
    class GhostPanel final : public juce::Component {
    public:
        explicit GhostPanel(zlgui::UIState& base);

        void paintGhost(juce::Graphics& g);

        /**
         * advance the morph / fade animation (message thread, once per frame)
         * @param time_stamp current time in seconds
         */
        void updateAnimation(double time_stamp);

        /**
         * set the ghost band parameters (message thread)
         */
        void setGhost(zldsp::filter::FilterType filter_type, float freq, float gain, float q, size_t order,
                      float cursor_x);

        /**
         * hide the ghost band (message thread)
         */
        void clearGhost();

        /**
         * check whether the ghost parameters have changed (response thread)
         */
        bool checkUpdate() {
            return to_update_.check();
        }

        /**
         * load the ghost parameters (response thread)
         * @return whether the ghost is on
         */
        bool getParas(zldsp::filter::FilterParameters& para, float& cursor_x, std::uint32_t& generation) const;

        /**
         * publish the target ghost curve (response thread)
         */
        void updateTarget(std::span<float> xs, std::span<float> dbs, float k, float b,
                          float glow_x, float glow_y, std::uint32_t generation);

    private:
        // time constant of the morph and fade animation in seconds
        static constexpr double kTimeConstant = .08;
        static constexpr float kConvergedPixel = .05f;
        static constexpr float kConvergedAlpha = .005f;

        struct GhostTarget {
            std::vector<float> xs, ys;
            juce::Point<float> glow;
            std::uint32_t generation{0};
        };

        zlgui::UIState& base_;

        std::atomic<bool> is_on_{false};
        std::atomic<int> filter_type_{0};
        std::atomic<float> freq_{1000.f}, gain_{0.f}, q_{.707f}, cursor_x_{0.f};
        std::atomic<size_t> order_{2};
        std::atomic<std::uint32_t> generation_{0};
        zlchore::thread::Notifier to_update_{};

        TriBuffer<GhostTarget> targets_{};

        // message thread animation state
        std::vector<float> xs_, ys_, target_ys_;
        juce::Point<float> glow_{}, target_glow_{};
        float alpha_{0.f};
        bool has_target_{false};
        bool is_converged_{true};
        std::uint32_t min_generation_{0};
        double previous_time_stamp_{-1.0};
        juce::Path path_;
    };
}
