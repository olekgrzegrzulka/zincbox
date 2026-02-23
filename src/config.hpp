#include <optional>
#include <string>
#include <variant>
#include "types.hpp"

using ConfigValue = std::variant<i32, float, std::string>;

void config_set_i32(std::string key, i32 value);
void config_set_float(std::string key, float value);
void config_set_string(std::string key, std::string value);

std::optional<i32> config_get_i32(std::string key);
std::optional<float> config_get_float(std::string key);
std::optional<std::string> config_get_string(std::string key);

void config_save_to_file(std::string file);
void config_load_from_file(std::string file);