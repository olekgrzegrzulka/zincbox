#include "core/player/equalizer.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace player {

  void Equalizer::init(u32 sample_rate) {
    if (sample_rate > 0) { m_sample_rate = sample_rate; }
    reset_state();
  }

  void Equalizer::reset_state() {
    for (auto& s : m_state) {
      s = BiquadState{};
    }
  }

  void Equalizer::set_gains(const EqualizerGains& gains_db) {
    bool all_flat = true;
    constexpr float Q_FACTOR = 1.41421356f;

    for (size_t i = 0; i < EQ_BAND_COUNT; ++i) {
      float db = std::clamp(gains_db[i], -12.0f, 12.0f);
      if (std::abs(db) >= 0.01f) {
        all_flat = false;
      } else {
        m_coeffs[i] = BiquadCoeffs{};
        continue;
      }

      float a = std::pow(10.0f, db / 40.0f);
      float w0 = 2.0f * std::numbers::pi_v<float> * EQ_FREQUENCIES[i] / static_cast<float>(m_sample_rate);
      float cos_w0 = std::cos(w0);
      float sin_w0 = std::sin(w0);
      float alpha = sin_w0 / (2.0f * Q_FACTOR);

      float b0 = 1.0f + (alpha * a);
      float b1 = -2.0f * cos_w0;
      float b2 = 1.0f - (alpha * a);
      float a0 = 1.0f + (alpha / a);
      float a1 = -2.0f * cos_w0;
      float a2 = 1.0f - (alpha / a);

      float inv_a0 = 1.0f / a0;
      m_coeffs[i].b0 = b0 * inv_a0;
      m_coeffs[i].b1 = b1 * inv_a0;
      m_coeffs[i].b2 = b2 * inv_a0;
      m_coeffs[i].a1 = a1 * inv_a0;
      m_coeffs[i].a2 = a2 * inv_a0;
    }

    m_bypass = all_flat;
  }

  void Equalizer::process_stereo(std::span<float> interleaved_frames) {
    if (m_bypass) { return; }

    size_t sample_count = interleaved_frames.size();
    for (size_t i = 0; i + 1 < sample_count; i += 2) {
      float left = interleaved_frames[i];
      float right = interleaved_frames[i + 1];

      for (size_t band = 0; band < EQ_BAND_COUNT; ++band) {
        const auto& c = m_coeffs[band];
        auto& s = m_state[band];

        float out_l = (c.b0 * left) + s.z1_l;
        s.z1_l = (c.b1 * left) - (c.a1 * out_l) + s.z2_l;
        s.z2_l = (c.b2 * left) - (c.a2 * out_l);
        left = out_l;

        float out_r = (c.b0 * right) + s.z1_r;
        s.z1_r = (c.b1 * right) - (c.a1 * out_r) + s.z2_r;
        s.z2_r = (c.b2 * right) - (c.a2 * out_r);
        right = out_r;
      }

      interleaved_frames[i] = std::clamp(left, -1.0f, 1.0f);
      interleaved_frames[i + 1] = std::clamp(right, -1.0f, 1.0f);
    }
  }

} // namespace player
