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

#include "../../chore/updater/fork_updater.hpp"
#include "../helper/panel_constants.hpp"

namespace zlpanel {
    // small pill below the top panel: "Update available (fork-N)  Install  Later"
    class UpdateBanner final : public juce::Component {
    public:
        explicit UpdateBanner(zlgui::UIState& base);

        // starts the (at most once per 24 h) background check; call when the editor shows
        void startCheck();

        // places the banner at the top right of its parent, below the top panel
        void updateLayout();

        void paint(juce::Graphics& g) override;

        void mouseMove(const juce::MouseEvent& event) override;

        void mouseExit(const juce::MouseEvent& event) override;

        void mouseDown(const juce::MouseEvent& event) override;

    private:
        enum Button {
            kNone,
            kInstall,
            kLater,
            kOK
        };

        zlgui::UIState& base_;
        bool started_{false};
        bool checking_{false};
        bool dismissed_{false};
        Button hover_{kNone};
        juce::Rectangle<int> text_rect_, install_rect_, later_rect_, ok_rect_;
        juce::String message_;
        // declared last: destroyed (and its thread joined) first
        zlupdate::ForkUpdater updater_;

        void onUpdaterChange();

        void refresh();

        Button getButtonAt(juce::Point<int> position) const;

        juce::Font getFont() const;

        static int getCurrentHour();
    };
}
