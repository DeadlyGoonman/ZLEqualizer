// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "freq_readout_panel.hpp"
#include "../../helper/freq_note.hpp"

namespace zlpanel {
    FreqReadoutPanel::FreqReadoutPanel(zlgui::UIState& base) :
        base_(base) {
        setInterceptsMouseClicks(false, false);
    }

    void FreqReadoutPanel::paint(juce::Graphics& g) {
        const auto bound = getLocalBounds().toFloat();
        g.setColour(base_.gui.getBackgroundColour().withAlpha(.9f));
        g.fillRoundedRectangle(bound, base_.gui.getFontSize() * .3f);
        g.setColour(base_.gui.getTextColour().withAlpha(.85f));
        g.setFont(juce::FontOptions(base_.gui.getFontSize() * 1.1f));
        g.drawText(text_, bound, juce::Justification::centred, false);
    }

    int FreqReadoutPanel::setFrequency(const double frequency) {
        const auto font = juce::Font(juce::FontOptions(base_.gui.getFontSize() * 1.1f));
        const auto freq = std::max(frequency, 1.0);
        const auto tenths = static_cast<int>(std::round(freq * 10.0));
        const auto semitones = 12.0 * std::log2(freq / freq_note::kA4Frequency);
        const auto cents = static_cast<int>(std::round(semitones * 100.0));
        const auto key = freq < 100.0 ? tenths : static_cast<int>(std::round(freq));
        if (key != tenths_ || cents != cents_key_) {
            tenths_ = key;
            cents_key_ = cents;
            juce::String freq_text;
            if (freq < 100.0) {
                freq_text = juce::String(tenths * .1, 1) + " Hz";
            } else if (freq < 1000.0) {
                freq_text = juce::String(static_cast<int>(std::round(freq))) + " Hz";
            } else {
                freq_text = juce::String(freq * .001, 2) + " kHz";
            }
            const auto note_index = static_cast<int>(std::round(semitones));
            const auto note_cents = cents - note_index * 100;
            const auto midi = note_index + 69;
            const auto note_text = juce::String(freq_note::kNoteNames[static_cast<size_t>(((midi % 12) + 12) % 12)])
                + juce::String(static_cast<int>(std::floor(static_cast<double>(midi) / 12.0)) - 1)
                + " " + (note_cents >= 0 ? "+" : "") + juce::String(note_cents) + "ct";
            text_ = freq_text + "  " + note_text;
            repaint();
        }
        return juce::GlyphArrangement::getStringWidth(font, text_) > 0.f
            ? static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(font, text_)
                                         + base_.gui.getFontSize() * .8f))
            : 0;
    }
}
