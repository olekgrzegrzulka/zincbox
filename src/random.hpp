#pragma once
#include <limits>
#include <random>
#include <span>
#include <type_traits>
#include "types.hpp"

class Random {
  private:
    std::default_random_engine rng;

  public:
    Random() {
      std::random_device rd;
      rng.seed(rd());
    }

    Random(i32 seed) {
      rng.seed(seed);
    }

    template <typename T>
      requires std::is_integral_v<T>
    T next(T from = std::numeric_limits<T>::min(), T to = std::numeric_limits<T>::max()) {
      std::uniform_int_distribution<T> distribution(from, to);
      return distribution(rng);
    }

    template <typename T>
      requires std::is_floating_point_v<T>
    T next(T from = 0.0, T to = 1.0) {
      std::uniform_real_distribution<T> distribution(from, to);
      return distribution(rng);
    }

    template <typename T>
      requires std::is_integral_v<T>
    T rand_sign() {
      std::uniform_int_distribution<T> distribution(static_cast<T>(0), static_cast<T>(1));
      return distribution(rng) * static_cast<T>(2) - static_cast<T>(1);
    }

    template <class T>
    T pick(std::span<T> values, std::span<int> weights) {
      ensure(values.size() == weights.size());

      std::discrete_distribution<int> distribution(weights.begin(), weights.end());
      return values[distribution(rng)];
    }

    bool rand_bool() {
      return next<i32>(0, 1) == 1;
    }
};

namespace StaticRandom {
  inline Random& get() {
    thread_local Random r;
    return r;
  }
}; // namespace StaticRandom
