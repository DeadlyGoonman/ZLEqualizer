// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "ghost_panel.hpp"

namespace zlpanel {
    GhostPanel::GhostPanel(zlgui::UIState& base) :
        base_(base) {
        setInterceptsMouseClicks(false, false);
    }

    void GhostPanel::paintGhost(juce::Graphics& g) {
        if (alpha_ < .001f || path_.isEmpty()) {
            return;
        }
        const auto font_size = base_.gui.getFontSize();
        // soft warm glow at the peak / cursor point
        const auto glow_radius = font_size * 2.5f;
        const auto glow_colour = base_.gui.getColourMap2(0);
        g.setGradientFill(juce::ColourGradient(glow_colour.withAlpha(.12f * alpha_), glow_,
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
        // pull the latest target curve
        if (targets_.pull()) {
            const auto& target{targets_.getReader()};
            if (target.generation >= min_generation_ && !target.ys.empty()) {
                xs_ = target.xs;
                target_ys_ = target.ys;
                target_glow_ = target.glow;
                // appear in place instead of morphing from a stale shape
                if (!has_target_ || alpha_ < .02f || ys_.size() != target_ys_.size()) {
                    ys_ = target_ys_;
                    glow_ = target_glow_;
                }
                has_target_ = true;
                is_converged_ = false;
            }
        }

        const auto dt = previous_time_stamp_ < 0.0
            ? 0.0
            : std::clamp(time_stamp - previous_time_stamp_, 0.0, .1);
        previous_time_stamp_ = time_stamp;
        const auto target_alpha = is_on_.load(std::memory_order::relaxed) && has_target_ ? 1.f : 0.f;
        if (is_converged_ && std::abs(alpha_ - target_alpha) < 1e-6f) {
            return;
        }
        // time-based exponential smoothing, independent of the refresh rate
        const auto portion = static_cast<float>(1.0 - std::exp(-dt / kTimeConstant));

        alpha_ += (target_alpha - alpha_) * portion;
        if (std::abs(target_alpha - alpha_) < kConvergedAlpha) {
            alpha_ = target_alpha;
        }

        if (!is_converged_) {
            float max_diff = 0.f;
            for (size_t i = 0; i < ys_.size(); ++i) {
                ys_[i] += (target_ys_[i] - ys_[i]) * portion;
                max_diff = std::max(max_diff, std::abs(target_ys_[i] - ys_[i]));
            }
            glow_ += (target_glow_ - glow_) * portion;
            max_diff = std::max(max_diff, glow_.getDistanceFrom(target_glow_));
            if (max_diff < kConvergedPixel) {
                ys_ = target_ys_;
                glow_ = target_glow_;
                is_converged_ = true;
            }
            path_.clear();
            PathMinimizer<1> minimizer(path_);
            minimizer.drawPath<true, false>(xs_, ys_);
        }
    }

    void GhostPanel::setGhost(const zldsp::filter::FilterType filter_type, const float freq, const float gain,
                              const float q, const size_t order, const float cursor_x) {
        const auto c_filter_type = static_cast<int>(filter_type);
        const auto was_on = is_on_.load(std::memory_order::relaxed);
        if (was_on
            && filter_type_.load(std::memory_order::relaxed) == c_filter_type
            && std::abs(freq_.load(std::memory_order::relaxed) - freq) < 1e-3f
            && std::abs(gain_.load(std::memory_order::relaxed) - gain) < 1e-3f
            && std::abs(q_.load(std::memory_order::relaxed) - q) < 1e-5f
            && order_.load(std::memory_order::relaxed) == order) {
            return;
        }
        filter_type_.store(c_filter_type, std::memory_order::relaxed);
        freq_.store(freq, std::memory_order::relaxed);
        gain_.store(gain, std::memory_order::relaxed);
        q_.store(q, std::memory_order::relaxed);
        order_.store(order, std::memory_order::relaxed);
        cursor_x_.store(cursor_x, std::memory_order::relaxed);
        const auto generation = generation_.load(std::memory_order::relaxed) + 1;
        generation_.store(generation, std::memory_order::release);
        if (!was_on) {
            // wait for a curve of the new parameters before fading in
            min_generation_ = generation;
            has_target_ = false;
        }
        is_on_.store(true, std::memory_order::relaxed);
        to_update_.signal();
    }

    void GhostPanel::clearGhost() {
        is_on_.store(false, std::memory_order::relaxed);
    }

    bool GhostPanel::getParas(zldsp::filter::FilterParameters& para, float& cursor_x,
                              std::uint32_t& generation) const {
        if (!is_on_.load(std::memory_order::relaxed)) {
            return false;
        }
        generation = generation_.load(std::memory_order::acquire);
        para.filter_type = static_cast<zldsp::filter::FilterType>(filter_type_.load(std::memory_order::relaxed));
        para.order = order_.load(std::memory_order::relaxed);
        para.freq = static_cast<double>(freq_.load(std::memory_order::relaxed));
        para.gain = static_cast<double>(gain_.load(std::memory_order::relaxed));
        para.q = static_cast<double>(q_.load(std::memory_order::relaxed));
        cursor_x = cursor_x_.load(std::memory_order::relaxed);
        return true;
    }

    void GhostPanel::updateTarget(const std::span<float> xs, const std::span<float> dbs, const float k, const float b,
                                  const float glow_x, const float glow_y, const std::uint32_t generation) {
        auto& target{targets_.getWriter()};
        target.xs.assign(xs.begin(), xs.end());
        target.ys.resize(dbs.size());
        for (size_t i = 0; i < dbs.size(); ++i) {
            target.ys[i] = std::fma(k, dbs[i], b);
        }
        target.glow = {glow_x, glow_y};
        target.generation = generation;
        targets_.publish();
    }
}
