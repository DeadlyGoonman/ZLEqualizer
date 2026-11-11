// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../../state/ui_state.hpp"

#include "../../../../PluginProcessor.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../multilingual/tooltip_helper.hpp"

#include "right_click_panel.hpp"
#include "../ghost_panel.hpp"

namespace zlpanel {
    class MouseEventPanel final : public juce::Component {
    public:
        explicit MouseEventPanel(PluginProcessor& p, zlgui::UIState& base,
                                 const multilingual::TooltipHelper& tooltip_helper,
                                 RightClickPanel &right_click_panel,
                                 GhostPanel& ghost_panel);

        ~MouseEventPanel() override;

        void mouseDown(const juce::MouseEvent& event) override;

        void mouseUp(const juce::MouseEvent& event) override;

        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void mouseEnter(const juce::MouseEvent& event) override;

        void mouseExit(const juce::MouseEvent& event) override;

        void mouseMove(const juce::MouseEvent& event) override;

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

        void mouseDrag(const juce::MouseEvent&) override;

        void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;

        void visibilityChanged() override;

        void updateBand();

        void updateSampleRate(double sample_rate);

        void repaintCallbackSlow();

        /**
         * @return whether the band has just been created by a single click (within a double-click time)
         */
        bool isJustCreated(size_t band) const;

        /**
         * @return whether the current/last press has created a band (and drags it)
         */
        bool isPlacementGesture() const { return placement_gesture_; }

    private:
        static constexpr int kFFTFreezeIdleUpdates{20};
        static constexpr float kPlacementQ{.707f};

        static constexpr std::array kInitIDs{
            zlp::PFilterStatus::kID, zlp::PFilterType::kID, zlp::PLRMode::kID,
            zlp::POrder::kID,
            zlp::PFreq::kID, zlp::PGain::kID, zlp::PQ::kID,
            zlp::PDynamicON::kID
        };

        PluginProcessor& p_ref_;
        zlgui::UIState& base_;
        zlgui::attachment::ComponentUpdater updater_{};
        RightClickPanel &right_click_panel_;
        GhostPanel& ghost_panel_;

        size_t previous_band_{zlp::kBandNum};
        bool is_click_armed_{false};
        bool placement_gesture_{false};
        size_t placing_band_{zlp::kBandNum};
        zldsp::filter::FilterType placing_type_{zldsp::filter::FilterType::kPeak};
        bool placing_gain_{false};
        band_helper::CutSlopeWheel cut_slope_wheel_{};
        size_t last_created_band_{zlp::kBandNum};
        juce::uint32 last_created_time_{0};

        std::atomic<float>& fft_freeze_ref_;
        bool is_shift_down_{false};
        bool shift_press_armed_{false};
        bool always_freeze_{false};
        bool c_fft_freeze_{false};
        bool is_fft_frozen_{false};
        bool has_mouse_position_{false};
        juce::Point<float> last_mouse_screen_position_{};
        int mouse_idle_updates_{0};

        float fft_max_{0.f};
        float slider_max_{0.f};

        zlgui::slider::SnappingSlider q_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> q_attachment_;

        std::atomic<float>* ftype_idx_ref_{};
        float c_ftype_idx_{-1.f};
        zlgui::slider::SnappingSlider slope_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> slope_attachment_;

        void updateFFTFreeze();

        void setFFTFreeze(bool should_freeze);

        void updateSlopeAttachment();

        /**
         * compute the initial parameters of the band created at the position
         * @return initial values in the order of kInitIDs
         */
        std::array<float, kInitIDs.size()> getInitValues(juce::Point<float> position, bool is_command_down,
                                                         float& max_db) const;

        float getFreq(float x) const;

        float getYPortion(float y) const;

        float getMaxDB() const;

        float getShelfOrPeakGain(zldsp::filter::FilterType type, float y_portion) const;

        /**
         * the band frequency when placing at x: peaks sit on the cursor, shelves are placed such that
         * the shelf plateau begins under the cursor
         */
        float getPlacementFreq(zldsp::filter::FilterType type, float x, float y_portion) const;

        void endPlacement();

        void updateGhost(juce::Point<float> position, bool is_command_down);

        /**
         * create a band at the position
         * @return whether a band has been created
         */
        bool createBand(juce::Point<float> position, bool is_command_down);
    };
}
