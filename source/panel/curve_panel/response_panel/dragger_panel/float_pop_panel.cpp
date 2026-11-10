// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "float_pop_panel.hpp"
#include "BinaryData.h"

namespace zlpanel {
    namespace {
        constexpr float kTextScale = 1.15f;
        constexpr float kTextWidthScale = 6.f;
        constexpr float kRowHeightScale = 1.5f;

        int getRowHeight(const float font_size) {
            return static_cast<int>(std::round(font_size * kRowHeightScale));
        }

        int getTextWidth(const float font_size) {
            return static_cast<int>(std::round(font_size * kTextWidthScale));
        }
    }

    FloatPopPanel::FloatPopPanel(PluginProcessor& p, zlgui::UIState& base,
                                 const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(),
        bypass_button_(base.gui,
                       juce::Drawable::createFromImageData(BinaryData::bypass_svg, BinaryData::bypass_svgSize),
                       true, tooltip_helper.getToolTipText(multilingual::kBandBypass)),
        solo_button_(base.gui,
                     juce::Drawable::createFromImageData(BinaryData::solo_svg, BinaryData::solo_svgSize),
                     true, tooltip_helper.getToolTipText(multilingual::kBandSolo)),
        close_button_(base.gui,
                      juce::Drawable::createFromImageData(BinaryData::close_svg, BinaryData::close_svgSize)),
        ftype_box_([]() -> std::vector<std::unique_ptr<juce::Drawable>> {
            std::vector<std::unique_ptr<juce::Drawable>> icons;
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::peak_svg, BinaryData::peak_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::lowshelf_svg, BinaryData::lowshelf_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::lowpass_svg, BinaryData::lowpass_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::highshelf_svg, BinaryData::highshelf_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::highpass_svg, BinaryData::highpass_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::notch_svg, BinaryData::notch_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::bandpass_svg, BinaryData::bandpass_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::tiltshelf_svg, BinaryData::tiltshelf_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::flatshelf_svg, BinaryData::flatshelf_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::allpass_svg, BinaryData::allpass_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::flatgain_svg, BinaryData::flatgain_svgSize));
            return icons;
        }(), base.gui, "", {}),
        lr_box_([]() -> std::vector<std::unique_ptr<juce::Drawable>> {
            std::vector<std::unique_ptr<juce::Drawable>> icons;
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::stereo_svg, BinaryData::stereo_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::left_svg, BinaryData::left_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::right_svg, BinaryData::right_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::mid_svg, BinaryData::mid_svgSize));
            icons.emplace_back(
                juce::Drawable::createFromImageData(BinaryData::side_svg, BinaryData::side_svgSize));
            return icons;
        }(), base.gui, "", {}),
        lr_chevron_(base.gui,
                    juce::Drawable::createFromImageData(BinaryData::chevron_down_svg,
                                                        BinaryData::chevron_down_svgSize)),
        freq_slider_("", base.gui),
        gain_slider_("", base.gui),
        q_slider_("", base.gui) {
        close_button_.setBufferedToImage(true);
        addAndMakeVisible(close_button_);
        close_button_.getButton().onClick = [this]() {
            if (const auto c_band = band_; c_band < zlp::kBandNum) {
                stopTimer();
                hover_band_ = zlp::kBandNum;
                if (c_band != base_.getSelectedBand()) {
                    // a hovered band is closed, keep the current selection
                    band_helper::turnOffBand(p_ref_, c_band);
                    attachBand(base_.getSelectedBand());
                    return;
                }
                band_helper::turnOffBand(p_ref_, c_band, base_.getSelectedBandSet());
                const auto band1 = band_helper::findClosestBand<true>(p_ref_, c_band);
                const auto band2 = band_helper::findClosestBand<false>(p_ref_, c_band);
                if (band1 < zlp::kBandNum) {
                    base_.setSelectedBand(band1);
                } else if (band2 < zlp::kBandNum) {
                    base_.setSelectedBand(band2);
                } else {
                    base_.setSelectedBand(zlp::kBandNum);
                }
                attachBand(base_.getSelectedBand());
            }
        };

        solo_button_.setImageAlpha(.5f, .75f, 1.f, 1.f);
        solo_button_.setBufferedToImage(true);
        addAndMakeVisible(solo_button_);
        solo_button_.getButton().onClick = [this]() {
            if (solo_button_.getButton().getToggleState()) {
                if (const auto c_band = band_; c_band < zlp::kBandNum) {
                    base_.setSoloWholeIdx(c_band);
                }
            } else {
                base_.setSoloWholeIdx(2 * zlp::kBandNum);
            }
        };

        bypass_button_.setImageAlpha(.5f, .75f, 1.f, 1.f);
        bypass_button_.setBufferedToImage(true);
        addAndMakeVisible(bypass_button_);
        bypass_button_.getButton().onClick = [this]() {
            if (const auto c_band = band_; c_band < zlp::kBandNum) {
                updateValue(p_ref_.parameters_.getParameter(zlp::PFilterStatus::kID + std::to_string(c_band)),
                            bypass_button_.getToggleState() ? 1.f : .5f);
            }
        };

        // names follow zlp::PFilterType, pass filters are called cuts
        ftype_box_.setIconListNames(juce::StringArray{
            "Peak", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
            "Band Pass", "Tilt Shelf", "Flat Shelf", "All Pass", "Flat Gain"
        });
        ftype_box_.getLAF().setFontScale(kTextScale);
        ftype_box_.getLAF().setOption(juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::upwards));
        ftype_box_.setBufferedToImage(true);
        addAndMakeVisible(ftype_box_);

        // opens downwards so the menu never lands on the delete (X) button
        const auto popup_option2 = juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::downwards).withMinimumNumColumns(5);
        lr_box_.setScrollEnabled(true);
        lr_box_.getLAF().setOption(popup_option2);
        lr_box_.setBufferedToImage(true);
        // the stereo mode box sits invisibly behind the chevron and opens its menu on click
        lr_box_.setAlpha(0.f);
        addAndMakeVisible(lr_box_);

        lr_chevron_.setImageAlpha(.5f, .5f);
        lr_chevron_.setInterceptsMouseClicks(false, false);
        lr_chevron_.setBufferedToImage(true);
        addAndMakeVisible(lr_chevron_);

        const auto setup_slider = [this](auto& slider) {
            slider.setFontScale(kTextScale);
            slider.setJustification(juce::Justification::centredRight);
            slider.getSlider().setDraggingEnabled(false);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.getSlider().setScrollWheelEnabled(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };

        setup_slider(freq_slider_);
        freq_slider_.permitted_characters_ = "-0123456789.kK#ABCDEFGabcdefg";
        freq_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value < 1000.0) {
                snprintf(buffer, sizeof(buffer), "%.2f Hz", value);
            } else {
                snprintf(buffer, sizeof(buffer), "%.2f kHz", value * 0.001);
            }
            return std::string{buffer};
        };
        freq_slider_.string_formatter_ = freq_note::getFrequencyFromNote;

        setup_slider(gain_slider_);
        gain_slider_.permitted_characters_ = "-+0123456789.";
        gain_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%+.2f dB", std::abs(value) < 0.005 ? 0.0 : value);
            return std::string{buffer};
        };

        setup_slider(q_slider_);
        q_slider_.permitted_characters_ = "0123456789.";
        q_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Q: %.3f", value);
            return std::string{buffer};
        };
        q_slider_.string_formatter_ = [](const std::string& text) -> std::optional<double> {
            const auto pos = text.find_first_of("0123456789.");
            if (pos == std::string::npos) {
                return std::nullopt;
            }
            return juce::String(text.substr(pos)).getDoubleValue();
        };

        base_.getSoloWholeIdxTree().addListener(this);
    }

    FloatPopPanel::~FloatPopPanel() {
        base_.getSoloWholeIdxTree().removeListener(this);
    }

    void FloatPopPanel::paint(juce::Graphics& g) {
        const auto font_size = base_.gui.getFontSize();
        const auto padding = static_cast<float>(getPaddingSize(font_size));
        const auto thickness = std::max(1.f, font_size * .06f);
        const auto body = getLocalBounds().toFloat().reduced(padding);
        juce::Path path;
        path.addRoundedRectangle(body, padding);

        const juce::DropShadow shadow{base_.gui.getTextColour().withAlpha(.25f), static_cast<int>(padding), {0, 0}};
        shadow.drawForPath(g, path);
        const auto background = base_.gui.getBackgroundColour();
        const auto border = base_.gui.getTextColour().withAlpha(.15f);
        g.setColour(background);
        g.fillPath(path);
        g.setColour(border);
        g.strokePath(path, juce::PathStrokeType(thickness));

        // small pointer towards the band
        if (notch_direction_ != 0) {
            const auto x = std::clamp(notch_x_, body.getX() + 2.f * padding, body.getRight() - 2.f * padding);
            const auto direction = static_cast<float>(notch_direction_);
            const auto base_y = notch_direction_ > 0 ? body.getY() : body.getBottom();
            const auto tip_y = base_y - direction * padding * .9f;
            juce::Path fill_notch;
            fill_notch.addTriangle(x - padding, base_y + direction * thickness,
                                   x, tip_y,
                                   x + padding, base_y + direction * thickness);
            g.setColour(background);
            g.fillPath(fill_notch);
            juce::Path notch;
            notch.startNewSubPath(x - padding, base_y);
            notch.lineTo(x, tip_y);
            notch.lineTo(x + padding, base_y);
            g.setColour(border);
            g.strokePath(notch, juce::PathStrokeType(thickness));
        }

        g.setColour(base_.gui.getTextColour().withAlpha(.35f));
        g.setFont(font_size);
        g.drawText("Ctrl", ctrl_bound_, juce::Justification::centred, false);
    }

    void FloatPopPanel::resized() {
        const auto font_size = base_.gui.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto button_size = getButtonSize(font_size);
        const auto row_height = getRowHeight(font_size);
        auto bound = getLocalBounds().reduced(2 * padding);

        bypass_button_.setOnColour(base_.gui.getColourMap2(0));
        bypass_button_.updateImages();

        // power | frequency | solo | close
        {
            auto row = bound.removeFromTop(row_height);
            bypass_button_.setBounds(row.removeFromLeft(button_size));
            close_button_.setBounds(row.removeFromRight(button_size));
            solo_button_.setBounds(row.removeFromRight(button_size));
            freq_slider_.setBounds(row);
        }
        // gain
        {
            auto row = bound.removeFromTop(row_height);
            row.removeFromLeft(button_size);
            row.removeFromRight(2 * button_size);
            gain_slider_.setBounds(row);
        }
        // filter type | Q | Ctrl | stereo mode
        {
            auto row = bound.removeFromTop(row_height);
            ftype_box_.setBounds(row.removeFromLeft(button_size).withSizeKeepingCentre(row_height, row_height));
            const auto chevron_bound = row.removeFromRight(button_size);
            lr_box_.setBounds(chevron_bound);
            lr_chevron_.setBounds(chevron_bound);
            ctrl_bound_ = row.removeFromRight(button_size);
            q_slider_.setBounds(row);
        }

        ideal_height_ = static_cast<float>(getIdealHeight());
        ideal_width_ = static_cast<float>(getIdealWidth());
    }

    void FloatPopPanel::updateBand() {
        attachBand(hover_band_ < zlp::kBandNum ? hover_band_ : base_.getSelectedBand());
    }

    void FloatPopPanel::attachBand(const size_t band) {
        if (band != band_) {
            band_ = band;
            if (band_ < zlp::kBandNum) {
                const auto band_s = std::to_string(band_);
                ftype_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                    ftype_box_.getBox(), p_ref_.parameters_, zlp::PFilterType::kID + band_s, updater_);
                lr_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                    lr_box_.getBox(), p_ref_.parameters_, zlp::PLRMode::kID + band_s, updater_);
                freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                    freq_slider_.getSlider(), p_ref_.parameters_, zlp::PFreq::kID + band_s, updater_);
                freq_slider_.setComponentID(zlp::PFreq::kID + band_s);
                freq_attachment_->updateComponent();
                freq_slider_.updateDisplayValue();
                gain_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                    gain_slider_.getSlider(), p_ref_.parameters_, zlp::PGain::kID + band_s, updater_);
                gain_attachment_->updateComponent();
                gain_slider_.updateDisplayValue();
                q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                    q_slider_.getSlider(), p_ref_.parameters_, zlp::PQ::kID + band_s, updater_);
                q_attachment_->updateComponent();
                q_slider_.updateDisplayValue();
                filter_status_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterStatus::kID + band_s);
                repaintCallBackSlow();
            } else {
                ftype_attachment_.reset();
                lr_attachment_.reset();
                freq_attachment_.reset();
                gain_attachment_.reset();
                q_attachment_.reset();
                freq_slider_.setComponentID("");
                filter_status_ptr_ = nullptr;
            }
            solo_button_.getButton().setToggleState(band_ < zlp::kBandNum && base_.getSoloWholeIdx() == band_,
                                                    juce::dontSendNotification);
            if (on_band_changed_) {
                on_band_changed_();
            }
        }
        setVisible(band_ < zlp::kBandNum);
    }

    void FloatPopPanel::setHoverBand(const size_t band) {
        stopTimer();
        hover_band_ = band;
        attachBand(band);
    }

    void FloatPopPanel::releaseHoverBand() {
        if (hover_band_ < zlp::kBandNum) {
            startTimer(kHoverReleaseMs);
        }
    }

    void FloatPopPanel::timerCallback() {
        // keep the popup while it is used
        if (is_dragging_ || isMouseOver(true) || hasKeyboardFocus(true) || ftype_box_.getBox().isPopupActive()
            || lr_box_.getBox().isPopupActive()) {
            return;
        }
        stopTimer();
        hover_band_ = zlp::kBandNum;
        attachBand(base_.getSelectedBand());
    }

    void FloatPopPanel::repaintCallBackSlow() {
        if (filter_status_ptr_ != nullptr) {
            if (band_ == hover_band_ && filter_status_ptr_->load(std::memory_order::relaxed) < .5f) {
                // the hovered band has been turned off
                stopTimer();
                hover_band_ = zlp::kBandNum;
                attachBand(base_.getSelectedBand());
                return;
            }
            updater_.updateComponents();
            const auto filter_on = filter_status_ptr_->load(std::memory_order::relaxed) > 1.5f;
            if (filter_on != bypass_button_.getToggleState()) {
                bypass_button_.getButton().setToggleState(filter_on, juce::dontSendNotification);
            }
        }
    }

    int FloatPopPanel::getIdealWidth() const {
        const auto font_size = base_.gui.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto button_size = getButtonSize(font_size);
        return 3 * button_size + getTextWidth(font_size) + 4 * padding;
    }

    int FloatPopPanel::getIdealHeight() const {
        const auto font_size = base_.gui.getFontSize();
        const auto padding = getPaddingSize(font_size);
        return 3 * getRowHeight(font_size) + 4 * padding;
    }

    void FloatPopPanel::setTargetVisible(const bool is_target_visible) {
        if (is_target_visible != is_target_visible_) {
            is_target_visible_ = is_target_visible;
            updateTransformation();
        }
    }

    void FloatPopPanel::updatePosition(const juce::Point<float> position,
                                       const juce::Point<float> target_position) {
        if (std::abs(position.x - position_.x) > .01f
            || std::abs(position.y - position_.y) > .01f
            || std::abs(target_position.x - target_position_.x) > .01f
            || std::abs(target_position.y - target_position_.y) > .01f) {
            position_ = position;
            target_position_ = target_position;
            updateTransformation();
        }
    }

    void FloatPopPanel::updateFloatingBound(const juce::Rectangle<float> bound) {
        const auto width = ideal_width_;
        const auto height = ideal_height_;
        const auto padding = base_.gui.getFontSize() * kPaddingScale * 1.5f;

        upper_center_ = {width * .5f, -base_.gui.getFontSize()};
        lower_center_ = {width * .5f, height + base_.gui.getFontSize()};
        left_center_ = {-padding, ideal_height_ * .5f};
        right_center_ = {width + padding, ideal_height_ * .5f};

        x_min_ = width * .5f;
        x_mid_ = bound.getWidth() * .5f;
        x_max_ = bound.getWidth() - width * .5f;
        y_min_ = ideal_height_ * .5f;
        y_max_ = bound.getHeight() - height * .5f;
        floating_top_ = bound.getY();
        floating_bottom_ = bound.getBottom();

        y1_ = bound.getHeight() * .25f;
        y2_ = bound.getHeight() * .5f + .5f;
        y3_ = bound.getHeight() * .75f;

        updateTransformation();
    }

    void FloatPopPanel::updateTransformation() {
        const auto place_below = [this]() {
            const auto tx = std::clamp(position_.x, x_min_, x_max_) - upper_center_.x;
            setTransform(juce::AffineTransform::translation(
                tx,
                position_.y - upper_center_.y));
            setNotch(1, position_.x - tx);
        };
        const auto place_above = [this]() {
            const auto tx = std::clamp(position_.x, x_min_, x_max_) - lower_center_.x;
            setTransform(juce::AffineTransform::translation(
                tx,
                position_.y - lower_center_.y));
            setNotch(-1, position_.x - tx);
        };
        const auto place_on_side = [this]() {
            setNotch(0, 0.f);
            if (position_.x < x_mid_) {
                setTransform(juce::AffineTransform::translation(
                    position_.x - left_center_.x,
                    std::clamp(position_.y, y_min_, y_max_) - left_center_.y));
            } else {
                setTransform(juce::AffineTransform::translation(
                    position_.x - right_center_.x,
                    std::clamp(position_.y, y_min_, y_max_) - right_center_.y));
            }
        };
        const auto place_using_default_rule = [this, &place_below, &place_above]() {
            if (position_.y < y1_ || (position_.y > y2_ && position_.y < y3_)) {
                place_below();
            } else {
                place_above();
            }
        };

        constexpr float target_position_epsilon = 1.f;
        const auto target_y_delta = target_position_.y - position_.y;
        if (!is_target_visible_ || std::abs(target_y_delta) <= target_position_epsilon) {
            place_using_default_rule();
        } else if (target_y_delta < 0.f) {
            const auto popup_bottom = position_.y - upper_center_.y + ideal_height_;
            if (popup_bottom <= floating_bottom_) {
                place_below();
            } else {
                place_on_side();
            }
        } else {
            const auto popup_top = position_.y - lower_center_.y;
            if (popup_top >= floating_top_) {
                place_above();
            } else {
                place_on_side();
            }
        }
    }

    void FloatPopPanel::setNotch(const int direction, const float x) {
        if (direction != notch_direction_) {
            // open the type list away from the band dot: popup below the dot opens downwards
            ftype_box_.getLAF().setOption(juce::PopupMenu::Options().withPreferredPopupDirection(
                direction > 0
                    ? juce::PopupMenu::Options::PopupDirection::downwards
                    : juce::PopupMenu::Options::PopupDirection::upwards));
        }
        if (direction != notch_direction_ || std::abs(x - notch_x_) > .5f) {
            notch_direction_ = direction;
            notch_x_ = x;
            repaint();
        }
    }

    void FloatPopPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {
        const auto solo_whole_idx = base_.getSoloWholeIdx();
        solo_button_.getButton().setToggleState(band_ < zlp::kBandNum && solo_whole_idx == band_,
                                                juce::dontSendNotification);
    }
}
