#pragma once
#include <istream>
#include <ostream>
#include <string>
#include "common/types.hpp"

template <typename T>
inline void write_bin(std::ostream& os, const T& v) {
  os.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

template <typename T>
inline void write_str(std::ostream& os, const std::basic_string<T>& s) {
  u64 count = s.size();
  write_bin(os, count);
  os.write(reinterpret_cast<const char*>(s.data()), count * sizeof(T));
}

template <typename T>
inline void read_bin(std::istream& is, T& v) {
  is.read(reinterpret_cast<char*>(&v), sizeof(T));
}

template <typename T>
inline void read_str(std::istream& is, std::basic_string<T>& s) {
  u64 count;
  read_bin(is, count);
  s.resize(count);
  if (count > 0) {
    is.read(reinterpret_cast<char*>(&s[0]), count * sizeof(T));
  }
}
