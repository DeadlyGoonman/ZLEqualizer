// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../state/ui_state.hpp"

#include "../../gui/gui.hpp"
#include "../../PluginProcessor.hpp"
#include "../helper/panel_constants.hpp"

namespace zlpanel {
    class ColourSettingPanel final : public juce::Component,
                                     private juce::ComboBox::Listener {
    public:
        static constexpr float kHeightP = 56.f;

        explicit ColourSettingPanel(PluginProcessor &p, zlgui::UIState &base);

        ~ColourSettingPanel() override;

        void loadSetting();

        void saveSetting();

        void resetSetting();

        int getIdealHeight() const;

        void resized() override;

    private:
        PluginProcessor &pRef;
        zlgui::UIState &base_;
        zlgui::label::NameLookAndFeel name_laf_;

        static constexpr size_t kNumSelectors = static_cast<size_t>(zlgui::ColourIdx::kColourNum);
        std::array<juce::Label, kNumSelectors> selector_labels_;
        std::array<std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector>, kNumSelectors> selectors_;

        // overrides, opacity 0 means "use the colour map"
        static constexpr size_t kNumBands = zlstate::kBandNum;
        juce::Label sum_line_label_;
        std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector> sum_line_selector_;
        juce::Label ghost_glow_label_;
        std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector> ghost_glow_selector_;
        juce::Label phase_curve_label_;
        std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector> phase_curve_selector_;
        juce::Label grad_top_label_, grad_bottom_label_, grad_label_;
        std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector> grad_top_selector_, grad_bottom_selector_;
        // brightness, where the colours meet (0 top, 1 bottom), width of the transition
        zlgui::slider::CompactLinearSlider<true, true, true> grad_brightness_slider_, grad_position_slider_,
                                                             grad_softness_slider_;
        // one row: pick a band, edit its colour; edits are cached per band until saved
        juce::Label band_label_;
        zlgui::combobox::CompactCombobox band_pick_box_;
        std::unique_ptr<zlgui::colour_selector::ColourOpacitySelector> band_selector_;
        std::array<juce::Colour, kNumBands> band_cache_{};
        size_t band_pick_idx_{0};

        static juce::StringArray getBandChoices();

        void comboBoxChanged(juce::ComboBox* box) override;

        juce::Label c_map1_label_, c_map2_label_;
        zlgui::colour_selector::ColourMapSelector c_map1_selector_, c_map2_selector_;

        std::unique_ptr<juce::FileChooser> chooser_;
        inline auto static const kSettingDirectory =
                juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Audio")
                .getChildFile("Presets")
                .getChildFile(JucePlugin_Manufacturer)
                .getChildFile("Shared Settings");
    };
}
