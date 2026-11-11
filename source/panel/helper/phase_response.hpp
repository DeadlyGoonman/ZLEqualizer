// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <vector>

#include "../../dsp/filter/ideal_filter/ideal.hpp"
#include "../../dsp/filter/iir_filter/tdf/tdf.hpp"
#include "../../zlp/sample_rate_helper.hpp"

namespace zlpanel::phase {
    /**
     * how the phase of the whole EQ is built, follows the filter structure parameter
     * kIIR: Minimum Phase / State Variable / Parallel, the phase of the cascaded digital biquads
     * kMatch / kMixed / kZero: the biquads times the FIR correction of zldsp::filter::*Calculator
     */
    enum class Mode { kIIR, kMatch, kMixed, kZero };

    inline Mode getModeFromStructure(const int structure_idx) {
        switch (structure_idx) {
        case 3: return Mode::kMatch;
        case 4: return Mode::kMixed;
        case 5: return Mode::kZero;
        default: return Mode::kIIR;
        }
    }

    /**
     * computes the complex frequency response of the enabled bands on the UI side (never the audio thread)
     * all buffers are allocated in prepare / compute at the first use, not on the audio thread
     * @tparam kFilterSize the number of cascading filters of a band
     * @tparam kBandNum the number of bands
     */
    template <size_t kFilterSize, size_t kBandNum>
    class PhaseCalculator {
    public:
        using IdealFilter = zldsp::filter::Ideal<float, kFilterSize>;
        using DigitalFilter = zldsp::filter::TDF<float, kFilterSize>;

        // FFT order at 50 kHz of the FIR corrections, see Controller (match 9, mixed 10, zero 13)
        static constexpr size_t kMatchOrder = 9, kMixedOrder = 10, kZeroOrder = 13;
        // decay region of the corrections in FIR bins, see zldsp::filter::*Calculator
        static constexpr double kMatchStart = 2., kMatchEnd = 32.;
        static constexpr double kMixedStart = 16., kMixedEnd = 128.;
        static constexpr double kZeroStart = 1., kZeroEnd = 32.;
        static constexpr double kMinMagSqr = 1e-16;

        void prepare(const double sample_rate) {
            sample_rate_ = sample_rate;
            for (size_t i = 0; i < kBandNum; ++i) {
                ideals_[i].prepare(sample_rate);
                tdfs_[i].prepare(sample_rate, 1, 0);
            }
        }

        /**
         * @param band
         * @param paras the parameters of the band, a scaled pass is replaced by the unscaled one
         */
        void setBand(const size_t band, zldsp::filter::FilterParameters paras) {
#if ZL_ENABLE_FILTER_SCALED_PASS
            if (paras.filter_type == zldsp::filter::kScaledLowPass) {
                paras.filter_type = zldsp::filter::kLowPass;
            } else if (paras.filter_type == zldsp::filter::kScaledHighPass) {
                paras.filter_type = zldsp::filter::kHighPass;
            }
#endif
            ideals_[band].forceUpdate(paras);
            tdfs_[band].forceUpdate(paras);
        }

        /**
         * compute the total phase of the listed bands
         * @param bands the bands whose responses are multiplied
         * @param mode
         * @param ws digital frequencies in rad / sample, ascending
         * @param phases the continuous (unwrapped) phase in rad, 0 at the first point is not assumed
         */
        void compute(const std::span<const size_t> bands, const Mode mode,
                     const std::span<const float> ws, const std::span<float> phases) {
            const auto n = ws.size();
            resize(n);
            for (size_t i = 0; i < n; ++i) {
                const auto w = static_cast<double>(ws[i]);
                w_[i] = w;
                wi_real_[i] = std::cos(w);
                wi_imag_[i] = -std::sin(w);
                total_real_[i] = 1.0;
                total_imag_[i] = 0.0;
            }
            double bin_scale = 0., start = 0., end = 0.;
            if (mode != Mode::kIIR) {
                const auto order = mode == Mode::kMatch ? kMatchOrder
                    : (mode == Mode::kMixed ? kMixedOrder : kZeroOrder);
                const auto fft_size = static_cast<double>(static_cast<size_t>(1) << zlp::getScaledOrder(
                                                              sample_rate_, order));
                // bin index = w / pi * (num_bin - 1), num_bin - 1 = fft_size / 2
                bin_scale = fft_size * .5 / std::numbers::pi;
                start = mode == Mode::kMatch ? kMatchStart : (mode == Mode::kMixed ? kMixedStart : kZeroStart);
                end = mode == Mode::kMatch ? kMatchEnd : (mode == Mode::kMixed ? kMixedEnd : kZeroEnd);
            }
            for (const auto band : bands) {
                auto& ideal = ideals_[band];
                auto& tdf = tdfs_[band];
                const auto filter_num = ideal.getFilterNum();
                for (size_t idx = 0; idx < filter_num; ++idx) {
                    zldsp::filter::TDFBase<double>::updateResponse(
                        tdf.getCoeff()[idx], wi_real_, wi_imag_, bq_real_, bq_imag_);
                    if (mode != Mode::kIIR) {
                        if (ideal.usesFirstOrderSections()) {
                            const auto& c = ideal.getCoeff()[idx];
                            zldsp::filter::IdealBase<double>::updateResponse(
                                std::array<double, 3>{c[0], c[2], c[3]}, w_, proto_real_, proto_imag_);
                        } else {
                            zldsp::filter::IdealBase<double>::updateResponse(
                                ideal.getCoeff()[idx], w_, proto_real_, proto_imag_);
                        }
                    }
                    for (size_t i = 0; i < n; ++i) {
                        double h_real = bq_real_[i], h_imag = bq_imag_[i];
                        if (mode != Mode::kIIR) {
                            applyCorrection(mode, static_cast<double>(w_[i]) * bin_scale, start, end,
                                            proto_real_[i], proto_imag_[i], h_real, h_imag);
                        }
                        const auto t_real = total_real_[i], t_imag = total_imag_[i];
                        total_real_[i] = t_real * h_real - t_imag * h_imag;
                        total_imag_[i] = t_real * h_imag + t_imag * h_real;
                    }
                }
            }
            // unwrap along the frequency axis
            double offset = 0., previous = 0.;
            for (size_t i = 0; i < n; ++i) {
                const auto arg = std::atan2(total_imag_[i], total_real_[i]);
                if (i > 0) {
                    const auto delta = arg - previous;
                    if (delta > std::numbers::pi) {
                        offset -= 2. * std::numbers::pi;
                    } else if (delta < -std::numbers::pi) {
                        offset += 2. * std::numbers::pi;
                    }
                }
                previous = arg;
                phases[i] = static_cast<float>(arg + offset);
            }
        }

        [[nodiscard]] double getSampleRate() const { return sample_rate_; }

    private:
        double sample_rate_{48000.};
        std::array<IdealFilter, kBandNum> ideals_{};
        std::array<DigitalFilter, kBandNum> tdfs_{};
        std::vector<double> w_, wi_real_, wi_imag_;
        std::vector<double> proto_real_, proto_imag_, bq_real_, bq_imag_;
        std::vector<double> total_real_, total_imag_;

        void resize(const size_t n) {
            if (w_.size() == n) {
                return;
            }
            for (auto* v : {&w_, &wi_real_, &wi_imag_, &proto_real_, &proto_imag_,
                            &bq_real_, &bq_imag_, &total_real_, &total_imag_}) {
                v->assign(n, 0.0);
            }
        }

        // the same smooth ramp over log(bin) as the correction calculators, evaluated at a continuous bin
        static double getDecay(const double bin, const double start, const double end) {
            if (bin <= start) {
                return 0.;
            }
            const auto t = std::clamp(std::log(bin / start) / std::log(end / start), 0., 1.);
            return t * t * (3. - 2. * t);
        }

        /**
         * the response of one section after the FIR correction, h = biquad * correction
         * (bq in / out in h, proto is the ideal prototype section)
         */
        static void applyCorrection(const Mode mode, const double bin, const double start, const double end,
                                    const double p_real, const double p_imag, double& h_real, double& h_imag) {
            const auto b_real = h_real, b_imag = h_imag;
            const auto b_mag_sqr = b_real * b_real + b_imag * b_imag;
            if (b_mag_sqr <= kMinMagSqr || bin <= start) {
                return;
            }
            const auto p_mag = std::sqrt(p_real * p_real + p_imag * p_imag);
            if (bin >= end) {
                if (mode == Mode::kMatch) {
                    h_real = p_real;
                    h_imag = p_imag;
                } else {
                    // mixed and zero phase end up on the real magnitude
                    h_real = p_mag;
                    h_imag = 0.;
                }
                return;
            }
            const auto k = getDecay(bin, start, end);
            if (mode == Mode::kZero) {
                // only the phase of the biquad is unwound in the decay region
                const auto theta = -std::atan2(b_imag, b_real) * k;
                h_real = b_real * std::cos(theta) - b_imag * std::sin(theta);
                h_imag = b_real * std::sin(theta) + b_imag * std::cos(theta);
                return;
            }
            // z = target / biquad, the target is the complex prototype (match) or its magnitude (mixed)
            const auto t_real = mode == Mode::kMatch ? p_real : p_mag;
            const auto t_imag = mode == Mode::kMatch ? p_imag : 0.;
            const auto z_real = (t_real * b_real + t_imag * b_imag) / b_mag_sqr;
            const auto z_imag = (t_imag * b_real - t_real * b_imag) / b_mag_sqr;
            const auto z_mag = std::sqrt(z_real * z_real + z_imag * z_imag);
            const auto z_arg = std::atan2(z_imag, z_real);
            const auto m = 1. + k * (z_mag - 1.);
            const auto theta = k * z_arg;
            const auto c_real = m * std::cos(theta), c_imag = m * std::sin(theta);
            h_real = b_real * c_real - b_imag * c_imag;
            h_imag = b_real * c_imag + b_imag * c_real;
        }
    };
}
