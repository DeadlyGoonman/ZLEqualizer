// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zlgui::slider {
    class SnappingSlider final : public juce::Slider {
    public:
        explicit SnappingSlider(UIBase& base, const juce::String& name = "") : juce::Slider(name), base_(base) {
        }

        void setValueWithGesture(const double value) {
            const auto new_value = getNormalisableRange().snapToLegalValue(value);
            if (!juce::approximatelyEqual(new_value, getValue())) {
                juce::Slider::ScopedDragNotification drag_notification{*this};
                setValue(new_value, juce::sendNotificationSync);
            }
        }

        /**
         * reset to the default (double click) value inside one change gesture, used by the middle mouse button
         */
        void resetToDefault() {
            if (!isEnabled()) {
                return;
            }
            const auto value = getDoubleClickReturnValue();
            if (value < getMinimum() || value > getMaximum()) {
                return;
            }
            if (!juce::approximatelyEqual(value, getValue())) {
                juce::Slider::ScopedDragNotification drag_notification{*this};
                setValue(value, juce::sendNotificationSync);
            }
        }

        void mouseDrag(const juce::MouseEvent& event) override {
            if (is_dragging_enabled_) {
                juce::Slider::mouseDrag(event);
            }
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& w) override {
            if (!isScrollWheelEnabled() || (event.mods.isAnyMouseButtonDown() && !is_graph_wheel_)) { return; }
            // avoid duplicate mousewheel events
            if (event.eventTime == last_wheel_time_) { return; }
            last_wheel_time_ = event.eventTime;
            // apply shift reverse
            juce::MouseWheelDetails wheel = w;
            if (event.mods.isShiftDown() && base_.getIsMouseWheelShiftReverse()) {
                wheel.deltaX = -wheel.deltaX;
                wheel.deltaY = -wheel.deltaY;
            }
            // multiply delta with sensitivity
            const auto sensitivity_mul = is_graph_wheel_
                                             ? (event.mods.isShiftDown()
                                                    ? base_.getSensitivity(kMouseWheelFine)
                                                    : base_.getSensitivity(kMouseWheel)) * kGraphWheelScale
                                             : (event.mods.isShiftDown()
                                                    ? base_.getKnobSensitivity(kMouseWheelFine)
                                                    : base_.getKnobSensitivity(kMouseWheel));
            cumulative_x_ += wheel.deltaX * sensitivity_mul;
            cumulative_y_ += wheel.deltaY * sensitivity_mul;
            // calculate delta value
            const auto current_value = getValue();
            const auto cumulative_wheel_delta = std::abs(cumulative_x_) > std::abs(cumulative_y_)
                                                    ? -cumulative_x_
                                                    : cumulative_y_;
            const auto delta = getMouseWheelDelta(current_value,
                                                  cumulative_wheel_delta * (wheel.isReversed ? -1.0f : 1.0f)
                                                  * (is_wheel_inverted_ ? -1.0f : 1.0f));
            // update slider value if delta value is larger than interval
            if (std::abs(delta) > getInterval() * 0.9) {
                cumulative_x_ = 0.f;
                cumulative_y_ = 0.f;
                juce::Slider::ScopedDragNotification drag_notification{*this};
                setValue(snapValue(current_value + delta, notDragging), juce::sendNotificationSync);
            }
        }

        void setDraggingEnabled(const bool f) {
            is_dragging_enabled_ = f;
        }

        /**
         * the wheel of a band on the graph: faster than knobs and also active while dragging
         */
        /** flips the wheel direction of this slider (used for the graph Q wheel) */
        void setWheelInverted(const bool f) {
            is_wheel_inverted_ = f;
        }

        void setGraphWheel(const bool f) {
            is_graph_wheel_ = f;
        }

    protected:
        UIBase& base_;
        float cumulative_x_{0.f}, cumulative_y_{0.f};
        juce::Time last_wheel_time_{};

        bool is_dragging_enabled_{true};

        static constexpr float kGraphWheelScale = 1.5f;
        bool is_graph_wheel_{false};
        bool is_wheel_inverted_{false};

        double getMouseWheelDelta(const double value, const float wheel_delta) {
            const auto proportion_delta = static_cast<double>(wheel_delta) * 0.15;
            const auto current_pos = valueToProportionOfLength(value);
            const auto new_pos = juce::jlimit(0.0, 1.0, current_pos + proportion_delta);
            if (std::abs(current_pos - 1.0) < 1e-6 && std::abs(new_pos - 1.0) < 1e-6) {
                cumulative_x_ = 0.f;
                cumulative_y_ = 0.f;
            }
            if (std::abs(current_pos - 0.0) < 1e-6 && std::abs(new_pos - 0.0) < 1e-6) {
                cumulative_x_ = 0.f;
                cumulative_y_ = 0.f;
            }
            return proportionOfLengthToValue(new_pos) - value;
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnappingSlider)
    };
}
