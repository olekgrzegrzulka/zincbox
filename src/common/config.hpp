#include "lib/json.cpp/json.h"

namespace config {
  void save_to_file();
  void load_from_file();

  jt::Json& json();
} // namespace config
