#pragma once
#include <istream>
#include <ostream>
#include <string>
#include <vector>
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

template <typename T>
inline void write_blob(std::ostream& os, const std::vector<T>& data) {
  write_bin(os, static_cast<u64>(data.size()));
  os.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
}

template <typename T>
inline void read_blob(std::istream& is, std::vector<T>& data) {
  u64 size;
  read_bin(is, size);
  data.resize(size);
  if (size == 0) { return; }
  is.read(reinterpret_cast<char*>(data.data()), size * sizeof(T));
}
