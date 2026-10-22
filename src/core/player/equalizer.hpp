#pragma once
#include <array>
#include <span>
#include "common/types.hpp"

namespace player {

  static inline constexpr size_t EQ_BAND_COUNT = 10;
  using EqualizerGains = std::array<float, EQ_BAND_COUNT>;

  static inline constexpr std::array<float, EQ_BAND_COUNT> EQ_FREQUENCIES = {
    31.25f, 62.5f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

  class Equalizer {
    public:
      void init(u32 sample_rate);
      void set_gains(const EqualizerGains& gains_db);
      void reset_state();
      void process_stereo(std::span<float> interleaved_frames);

    private:
      struct BiquadCoeffs {
          float b0 = 1.0f;
          float b1 = 0.0f;
          float b2 = 0.0f;
          float a1 = 0.0f;
          float a2 = 0.0f;
      };

      struct BiquadState {
          float z1_l = 0.0f;
          float z2_l = 0.0f;
          float z1_r = 0.0f;
          float z2_r = 0.0f;
      };

      u32 m_sample_rate = 48000;
      bool m_bypass = true;
      std::array<BiquadCoeffs, EQ_BAND_COUNT> m_coeffs{};
      std::array<BiquadState, EQ_BAND_COUNT> m_state{};
  };

} // namespace player
