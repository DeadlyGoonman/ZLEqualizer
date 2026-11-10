// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "update_banner.hpp"

namespace zlpanel {
    namespace {
        constexpr int kCheckIntervalHours = 24;
    }

    UpdateBanner::UpdateBanner(zlgui::UIState& base) :
        base_(base),
        updater_([this]() { onUpdaterChange(); }) {
        setOpaque(false);
        setVisible(false);
    }

    int UpdateBanner::getCurrentHour() {
        return static_cast<int>(juce::Time::currentTimeMillis() / (1000 * 60 * 60));
    }

    void UpdateBanner::startCheck() {
        if (started_ || !base_.getUpdateCheckEnabled()) {
            return;
        }
        started_ = true;
        const auto now = getCurrentHour();
        const auto last = base_.getUpdateLastCheckHour();
        if (last <= 0 || now < last || now - last >= kCheckIntervalHours) {
            checking_ = true;
            updater_.startCheck();
            return;
        }
        // checked recently: reuse what that check found
        const auto known = base_.getUpdateKnownBuild();
        if (known > zlupdate::kForkBuild) {
            updater_.setKnownRelease({known, zlupdate::getDefaultAssetURL(known), zlupdate::getPageURL(known)});
            refresh();
        }
    }

    void UpdateBanner::onUpdaterChange() {
        if (checking_) {
            checking_ = false;
            if (updater_.didServerReply()) {
                const auto release = updater_.getRelease();
                const auto found = updater_.getState() == zlupdate::ForkUpdater::State::kAvailable;
                base_.setUpdateLastCheckHour(getCurrentHour());
                base_.setUpdateKnownBuild(found ? release.build : 0);
                base_.requestUISettingSave();
            }
        }
        refresh();
    }

    void UpdateBanner::refresh() {
        using State = zlupdate::ForkUpdater::State;
        const auto state = updater_.getState();
        const auto build = updater_.getRelease().build;
        bool show = !dismissed_;
        switch (state) {
        case State::kAvailable:
            message_ = "Update available (fork-" + juce::String(build) + ")";
            show = show && build > base_.getUpdateDismissedBuild();
            break;
        case State::kDownloading:
            message_ = "Downloading update...";
            break;
        case State::kReady:
            message_ = "Update ready " + juce::String::charToString(0x2014) + " restart your DAW to finish";
            break;
        case State::kCancelled:
            message_ = "Update cancelled";
            break;
        case State::kFailed:
            message_ = "Update failed";
            break;
        case State::kIdle:
        case State::kChecking:
            show = false;
            break;
        }
        setVisible(show);
        if (show) {
            updateLayout();
            repaint();
        }
    }

    juce::Font UpdateBanner::getFont() const {
        return juce::Font{juce::FontOptions(base_.gui.getFontSize() * zlgui::kFontNormal)};
    }

    void UpdateBanner::updateLayout() {
        auto* parent = getParentComponent();
        if (parent == nullptr || !isVisible()) {
            return;
        }
        using State = zlupdate::ForkUpdater::State;
        const auto font_size = base_.gui.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto height = juce::roundToInt(font_size * 1.8f);
        const auto font = getFont();
        const auto text_width = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, message_));
        const auto state = updater_.getState();
        const auto button_width = [&font, padding](const juce::String& text) {
            return juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text)) + padding * 2;
        };

        int width = padding + text_width + padding;
        install_rect_ = later_rect_ = ok_rect_ = {};
        text_rect_ = {padding, 0, text_width + 2, height};
        if (state == State::kAvailable) {
            const auto install_w = button_width("Install");
            const auto later_w = button_width("Later");
            install_rect_ = {width, 0, install_w, height};
            later_rect_ = {width + install_w, 0, later_w, height};
            width += install_w + later_w + padding / 2;
        } else if (state != State::kDownloading) {
            const auto ok_w = button_width("OK");
            ok_rect_ = {width, 0, ok_w, height};
            width += ok_w + padding / 2;
        }
        const auto top = getTopPanelHeight(font_size) + padding / 2;
        setBounds(parent->getWidth() - width - padding, top, width, height);
    }

    void UpdateBanner::paint(juce::Graphics& g) {
        const auto bound = getLocalBounds().toFloat().reduced(.5f);
        const auto corner = bound.getHeight() * .5f;
        g.setColour(base_.gui.getBackgroundColour().withAlpha(.92f));
        g.fillRoundedRectangle(bound, corner);
        g.setColour(base_.gui.getTextColour().withAlpha(.3f));
        g.drawRoundedRectangle(bound, corner, 1.f);

        g.setFont(getFont());
        g.setColour(base_.gui.getTextColour().withAlpha(.85f));
        g.drawText(message_, text_rect_, juce::Justification::centredLeft, false);

        const auto draw_button = [this, &g](const Button button, const juce::Rectangle<int>& rect,
                                            const juce::String& text) {
            if (rect.isEmpty()) {
                return;
            }
            g.setColour(base_.gui.getTextColour().withAlpha(hover_ == button ? 1.f : .6f));
            g.drawText(text, rect, juce::Justification::centred, false);
        };
        draw_button(kInstall, install_rect_, "Install");
        draw_button(kLater, later_rect_, "Later");
        draw_button(kOK, ok_rect_, "OK");
    }

    UpdateBanner::Button UpdateBanner::getButtonAt(const juce::Point<int> position) const {
        if (install_rect_.contains(position)) {
            return kInstall;
        }
        if (later_rect_.contains(position)) {
            return kLater;
        }
        if (ok_rect_.contains(position)) {
            return kOK;
        }
        return kNone;
    }

    void UpdateBanner::mouseMove(const juce::MouseEvent& event) {
        const auto button = getButtonAt(event.getPosition());
        if (button != hover_) {
            hover_ = button;
            setMouseCursor(button == kNone ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
            repaint();
        }
    }

    void UpdateBanner::mouseExit(const juce::MouseEvent&) {
        if (hover_ != kNone) {
            hover_ = kNone;
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    void UpdateBanner::mouseDown(const juce::MouseEvent& event) {
        switch (getButtonAt(event.getPosition())) {
        case kInstall: {
#if JUCE_WINDOWS
            updater_.startInstall();
            refresh();
#else
            juce::URL(updater_.getRelease().page_url).launchInDefaultBrowser();
            dismissed_ = true;
            refresh();
#endif
            break;
        }
        case kLater: {
            base_.setUpdateDismissedBuild(updater_.getRelease().build);
            base_.requestUISettingSave();
            refresh();
            break;
        }
        case kOK: {
            dismissed_ = true;
            refresh();
            break;
        }
        case kNone:
            break;
        }
    }
}
