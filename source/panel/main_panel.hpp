// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU标志 General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../state/ui_state.hpp"

#include "control_panel/control_panel.hpp"
#include "control_panel/extra_dynamic_panel.hpp"
#include "curve_panel/curve_panel.hpp"
#include "top_panel/update_banner.hpp"
#include "preset_browser/preset_browser.hpp"
#include "ui_setting_panel/ui_setting_panel.hpp"

namespace zlpanel {
    class MainPanel final : public juce::Component,
                            private juce::ValueTree::Listener {
    public:
        explicit MainPanel(PluginProcessor& p, zlgui::UIState& base, multilingual::TooltipLanguage language);

        ~MainPanel() override;

        void resized() override;

        // every mouse press anywhere in the editor starts a new undo step
        void mouseDown(const juce::MouseEvent&) override;

        void repaintCallBack(double time_stamp);

        void startThreads();

        void stopThreads();

        ControlPanel& getControlPanel() {
            return control_panel_;
        }

        OutputPanel& getOutputPanel() {
            return curve_panel_.getOutputPanel();
        }

    private:
        static constexpr float kHoWMin = 0.33f;

        PluginProcessor& p_ref_;
        zlgui::UIState& base_;
        multilingual::TooltipHelper tooltip_helper_;

        RefreshHandler refresh_handler_;
        double previous_time_stamp_{-1.0};
        double refresh_rate_{-1.0};

        CurvePanel curve_panel_;
        ControlPanel control_panel_;
        UpdateBanner update_banner_;
        ExtraDynamicPanel extra_dynamic_panel_;
        PresetBrowser preset_browser_;
        UISettingPanel ui_setting_panel_;

        zlgui::tooltip::TooltipLookAndFeel tooltip_laf_;
        std::unique_ptr<zlgui::tooltip::TooltipWindow> tooltip_window_;

        size_t c_band_{zlp::kBandNum};
        double c_sample_rate_{0.};

        // the control panel glides horizontally to the selected band
        static constexpr double kControlGlideSeconds = .12;
        bool control_has_band_{false};
        float control_dx_{0.f};
        float applied_control_dx_{0.f};
        double control_time_stamp_{-1.0};

        void updateControlPosition(double time_stamp);

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;

        void repaintCallBackSlow();
    };
}
