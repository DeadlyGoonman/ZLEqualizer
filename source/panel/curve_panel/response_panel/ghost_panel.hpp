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
     * a faint preview of the band that would be created at the mouse position
     * the curve is computed on the message thread so that it follows the cursor without latency
     */
    class GhostPanel final : public juce::Component {
    public:
        explicit GhostPanel(zlgui::UIState& base, std::atomic<float>& gain_scale);

        void paintGhost(juce::Graphics& g);

        /**
         * advance the shape crossfade and the fade in/out (message thread, once per frame)
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
         * publish the drawing layout (response thread)
         */
        void updateLayout(std::span<float> xs, std::span<float> ws, float k, float b,
                          double sample_rate, double slider_max, float height);

    private:
        // duration of the shape crossfade in seconds
        static constexpr double kCrossfadeSeconds = .09;
        // duration of the fade in/out in seconds
        static constexpr double kFadeSeconds = .06;

        struct Layout {
            std::vector<float> xs, ws;
            float k{0.f}, b{0.f};
            double sample_rate{0.0}, slider_max{0.0};
            float height{0.f};
        };

        struct Shape {
            zldsp::filter::FilterType filter_type{zldsp::filter::FilterType::kPeak};
            size_t order{2};
            float gain{0.f};
        };

        zlgui::UIState& base_;
        std::atomic<float>& gain_scale_;

        TriBuffer<Layout> layouts_{};
        Layout layout_{};
        bool has_layout_{false};

        bool is_on_{false};
        Shape current_{};

        // shapes being blended, the last one is the current target, weights always sum to 1
        static constexpr size_t kMaxSlots = 4;
        struct Slot {
            Shape shape{};
            float weight{1.f};
            zldsp::filter::Ideal<float, zlp::Controller::kFilterSize> ideal{};
            zldsp::vector::aligned_vector<float> dbs{};
            // every shape keeps the frequency / q / cursor it was last updated with, so a fading shape stays put
            float freq{1000.f}, q{.707f}, cursor_x{0.f};
        };
        std::array<Slot, kMaxSlots> slots_{};
        // sampling grid locked to the center frequency of the newest shape, shared by all shapes so they can morph
        std::vector<float> grid_xs_{}, grid_ws_{};
        juce::Path path_{};
        juce::Point<float> glow_{};
        size_t slot_count_{1};
        // eased progress of the newest slot, 0..1
        float fade_t_{1.f};
        std::vector<float> ys_{};

        bool is_crossfading_{false};

        float alpha_{0.f};
        double previous_time_stamp_{-1.0};

        static double getNow();

        static float getButtonMag(zldsp::filter::FilterType filter_type, float gain);

        /** the glow sits at the cursor: the peak point, or the plateau of a shelf */
        static float getGlowMag(zldsp::filter::FilterType filter_type, float gain);

        float getScaledGain(float gain) const;

        void computeCurve(Slot& slot);

        void applyWeights();

        void computeGrid();

        void computeCurves();

        void rebuildPath();
    };
}
