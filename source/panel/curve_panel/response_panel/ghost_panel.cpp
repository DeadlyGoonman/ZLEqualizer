// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "ghost_panel.hpp"

#include <numbers>

namespace zlpanel {
    GhostPanel::GhostPanel(zlgui::UIState& base, std::atomic<float>& gain_scale) :
        base_(base), gain_scale_(gain_scale) {
        setInterceptsMouseClicks(false, false);
    }

    void GhostPanel::paintGhost(juce::Graphics& g) {
        if (alpha_ < .001f || path_.isEmpty()) {
            return;
        }
        const auto font_size = base_.gui.getFontSize();
        // soft warm glow at the peak / cursor point
        const auto glow_radius = font_size * 2.5f;
        const auto glow_colour = base_.getGhostGlowColour();
        g.setGradientFill(juce::ColourGradient(glow_colour.withAlpha(.12f * alpha_ * glow_colour.getFloatAlpha()), glow_,
                                               glow_colour.withAlpha(0.f),
                                               glow_.translated(glow_radius, 0.f), true));
        g.fillEllipse(juce::Rectangle<float>(2.f * glow_radius, 2.f * glow_radius).withCentre(glow_));
        // faint thin outline of the band
        g.setColour(base_.gui.getTextColour().withAlpha(.3f * alpha_));
        g.strokePath(path_, juce::PathStrokeType(font_size * .1f,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::butt));
    }

    void GhostPanel::updateAnimation(const double time_stamp) {
        // pick up a new layout (size, scale or sample rate change)
        if (layouts_.pull()) {
            const auto& layout{layouts_.getReader()};
            layout_.xs = layout.xs;
            layout_.ws = layout.ws;
            layout_.k = layout.k;
            layout_.b = layout.b;
            layout_.sample_rate = layout.sample_rate;
            layout_.slider_max = layout.slider_max;
            layout_.height = layout.height;
            has_layout_ = layout_.ws.size() > 1 && layout_.xs.size() == layout_.ws.size()
                && layout_.sample_rate > 1.0 && layout_.xs.back() > 1.f;
            if (is_on_ && has_layout_) {
                computeCurves();
                rebuildPath();
            }
        }

        const auto dt = previous_time_stamp_ < 0.0
            ? 0.0
            : std::clamp(time_stamp - previous_time_stamp_, 0.0, .1);
        previous_time_stamp_ = time_stamp;

        // linear fade in/out
        const auto target_alpha = is_on_ && has_layout_ ? 1.f : 0.f;
        if (std::abs(alpha_ - target_alpha) > 0.f) {
            const auto step = static_cast<float>(dt / kFadeSeconds);
            alpha_ = target_alpha > alpha_
                ? std::min(target_alpha, alpha_ + step)
                : std::max(target_alpha, alpha_ - step);
        }

        // eased crossfade into the newest shape; older shapes keep their relative weights, so the curve is continuous
        if (is_crossfading_) {
            fade_t_ = std::min(1.f, fade_t_ + static_cast<float>(dt / kCrossfadeSeconds));
            applyWeights();
            if (fade_t_ >= 1.f) {
                is_crossfading_ = false;
            }
            rebuildPath();
        }
    }

    void GhostPanel::applyWeights() {
        const auto t = fade_t_;
        const auto w_new = t * t * (3.f - 2.f * t);
        auto& newest = slots_[slot_count_ - 1];
        float others{0.f};
        for (size_t i = 0; i + 1 < slot_count_; ++i) {
            others += slots_[i].weight;
        }
        const auto scale = others > 1e-6f ? (1.f - w_new) / others : 0.f;
        for (size_t i = 0; i + 1 < slot_count_; ++i) {
            slots_[i].weight *= scale;
        }
        newest.weight = others > 1e-6f ? w_new : 1.f;
        // drop faded out shapes
        size_t kept{0};
        for (size_t i = 0; i < slot_count_; ++i) {
            if (i + 1 == slot_count_ || slots_[i].weight > .002f) {
                if (kept != i) {
                    std::swap(slots_[kept], slots_[i]);
                }
                ++kept;
            }
        }
        slot_count_ = kept;
    }

    void GhostPanel::setGhost(const zldsp::filter::FilterType filter_type, const float freq, const float gain,
                              const float q, const size_t order, const float cursor_x) {
        const auto was_on = is_on_ && alpha_ > .001f;
        is_on_ = true;
        const auto shape_changed = filter_type != current_.filter_type || order != current_.order;
        if (!was_on) {
            // appear in place, no morph from a stale shape
            slot_count_ = 1;
            slots_[0].weight = 1.f;
            is_crossfading_ = false;
            fade_t_ = 1.f;
        } else if (shape_changed) {
            // keep the shapes (and weights) currently on screen, fade the new one in on top
            if (slot_count_ == kMaxSlots) {
                // drop the faintest older shape
                size_t drop{0};
                for (size_t i = 1; i + 1 < slot_count_; ++i) {
                    if (slots_[i].weight < slots_[drop].weight) {
                        drop = i;
                    }
                }
                for (size_t i = drop; i + 1 < slot_count_; ++i) {
                    std::swap(slots_[i], slots_[i + 1]);
                }
                --slot_count_;
            }
            slots_[slot_count_].weight = 0.f;
            slots_[slot_count_].dbs.clear();
            ++slot_count_;
            is_crossfading_ = true;
            fade_t_ = 0.f;
        }
        current_.filter_type = filter_type;
        current_.order = order;
        current_.gain = gain;
        slots_[slot_count_ - 1].shape = current_;
        auto& newest = slots_[slot_count_ - 1];
        newest.freq = freq;
        newest.q = q;
        newest.cursor_x = cursor_x;
        if (has_layout_) {
            computeCurves();
            rebuildPath();
        }
    }

    void GhostPanel::clearGhost() {
        is_on_ = false;
    }

    void GhostPanel::updateLayout(const std::span<float> xs, const std::span<float> ws, const float k, const float b,
                                  const double sample_rate, const double slider_max, const float height) {
        auto& layout{layouts_.getWriter()};
        layout.xs.assign(xs.begin(), xs.end());
        layout.ws.assign(ws.begin(), ws.end());
        layout.k = k;
        layout.b = b;
        layout.sample_rate = sample_rate;
        layout.slider_max = slider_max;
        layout.height = height;
        layouts_.publish();
    }

    double GhostPanel::getNow() {
        return juce::Time::getMillisecondCounterHiRes() * .001;
    }

    float GhostPanel::getButtonMag(const zldsp::filter::FilterType filter_type, const float gain) {
        if (filter_type == zldsp::filter::kPeak || filter_type == zldsp::filter::kFlatGain) {
            return gain;
        } else if (filter_type == zldsp::filter::kLowShelf || filter_type == zldsp::filter::kHighShelf
            || filter_type == zldsp::filter::kTiltShelf || filter_type == zldsp::filter::kFlatTilt) {
            return .5f * gain;
        }
        return 0.f;
    }

    float GhostPanel::getGlowMag(const zldsp::filter::FilterType filter_type, const float gain) {
        if (filter_type == zldsp::filter::kLowShelf || filter_type == zldsp::filter::kHighShelf) {
            return gain;
        }
        return getButtonMag(filter_type, gain);
    }

    float GhostPanel::getScaledGain(const float gain) const {
        return std::clamp(gain * gain_scale_.load(std::memory_order::relaxed) / 100.f, -30.f, 30.f);
    }

    void GhostPanel::computeCurve(Slot& slot) {
        const auto& shape = slot.shape;
        auto& ideal = slot.ideal;
        auto& dbs = slot.dbs;
        zldsp::filter::FilterParameters para{};
        para.filter_type = shape.filter_type;
        para.order = shape.order;
        para.freq = std::min(static_cast<double>(slot.freq), layout_.slider_max);
        para.gain = static_cast<double>(getScaledGain(shape.gain));
        para.q = static_cast<double>(slot.q);
        dbs.resize(grid_ws_.size());
        ideal.prepare(layout_.sample_rate);
        ideal.forceUpdate(para);
        ideal.updateMagnitudeSquare(grid_ws_, dbs);
        zldsp::vector::sqr_mag_to_db(dbs.data(), dbs.size());
    }

    void GhostPanel::computeGrid() {
        // the layout maps x linearly to log(w), shift the grid so that one point sits on the center
        const auto num_points = layout_.xs.size();
        const auto width = layout_.xs.back();
        const auto w0 = static_cast<double>(layout_.ws.front());
        const auto log_ratio = std::log(static_cast<double>(layout_.ws.back()) / w0);
        const auto freq = std::min(static_cast<double>(slots_[slot_count_ - 1].freq), layout_.slider_max);
        const auto center_w = 2.0 * std::numbers::pi * freq / layout_.sample_rate;
        const auto center_x = static_cast<double>(width) * std::log(center_w / w0) / log_ratio;
        const auto dx = static_cast<double>(width) / static_cast<double>(num_points - 1);
        const auto phase = center_x - std::floor(center_x / dx) * dx;

        grid_xs_.clear();
        if (phase > 1e-3) {
            grid_xs_.emplace_back(0.f);
        }
        for (auto x = phase; x < static_cast<double>(width) - 1e-3; x += dx) {
            grid_xs_.emplace_back(static_cast<float>(x));
        }
        grid_xs_.emplace_back(width);
        grid_ws_.resize(grid_xs_.size());
        for (size_t i = 0; i < grid_xs_.size(); ++i) {
            grid_ws_[i] = static_cast<float>(
                w0 * std::exp(log_ratio * static_cast<double>(grid_xs_[i]) / static_cast<double>(width)));
        }
    }

    void GhostPanel::computeCurves() {
        computeGrid();
        for (size_t i = 0; i < slot_count_; ++i) {
            computeCurve(slots_[i]);
        }
    }

    void GhostPanel::rebuildPath() {
        if (!has_layout_) {
            return;
        }
        for (size_t s = 0; s < slot_count_; ++s) {
            if (slots_[s].dbs.size() != grid_xs_.size()) {
                return;
            }
        }
        const auto k = layout_.k;
        const auto b = layout_.b;
        // morph: the shapes are blended by weight, so one curve flows from the old shape into the new one
        ys_.resize(grid_xs_.size());
        for (size_t i = 0; i < ys_.size(); ++i) {
            float db{0.f};
            for (size_t s = 0; s < slot_count_; ++s) {
                // limit the deep stop bands, they would swallow the blend
                db += slots_[s].weight * std::clamp(slots_[s].dbs[i], -60.f, 60.f);
            }
            // floor the deep dips just below the panel, so notches always end at the same place
            ys_[i] = std::min(std::fma(k, db, b), layout_.height + 2.f);
        }
        // the glow slides with the weights from the old cursor point to the new one
        glow_ = {0.f, 0.f};
        for (size_t s = 0; s < slot_count_; ++s) {
            const auto& slot = slots_[s];
            const auto mag = getGlowMag(slot.shape.filter_type, getScaledGain(slot.shape.gain));
            glow_.x += slot.weight * slot.cursor_x;
            glow_.y += slot.weight * (k * mag + b);
        }
        path_.clear();
        PathMinimizer<1> minimizer(path_);
        minimizer.drawPath<true, false>(grid_xs_, ys_);
    }
}
