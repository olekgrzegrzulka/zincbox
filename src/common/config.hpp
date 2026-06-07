#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include "common/types.hpp"

using ConfigValue = std::variant<i32, float, std::string>;

void config_set_i32(std::string_view key, i32 value);
void config_set_float(std::string_view key, float value);
void config_set_string(std::string_view key, std::string value);

std::optional<i32> config_get_i32(std::string_view key);
std::optional<float> config_get_float(std::string_view key);
std::optional<std::string> config_get_string(std::string_view key);

void config_get_i32_if_set(std::string_view key, i32&);
void config_get_float_if_set(std::string_view key, float&);
void config_get_string_if_set(std::string_view key, std::string&);

void config_save_to_file(const std::string& file);
void config_load_from_file(const std::string& file);
