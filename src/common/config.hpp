#include <optional>
#include <string>
#include "common/types.hpp"
#include "lib/json.cpp/json.h"

namespace config {
  void set_i32(const std::string& key, i32 value);
  void set_float(const std::string& key, float value);
  void set_string(const std::string& key, std::string value);

  std::optional<i32> get_i32(const std::string& key);
  std::optional<float> get_float(const std::string& key);
  std::optional<std::string> get_string(const std::string& key);

  void save_to_file();
  void load_from_file();

  jt::Json& json();
} // namespace config
