#include <exception>
#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include "config.hpp"

struct string_hash {
  using is_transparent = void;
  [[nodiscard]] size_t operator()(const char* txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const std::string& txt) const {
    return std::hash<std::string>{}(txt);
  }
};

std::unordered_map<std::string, ConfigValue, string_hash, std::equal_to<>> map{};

void config_set_i32(std::string_view key, i32 value) {
  map[std::string{key}] = ConfigValue{value};
}

void config_set_float(std::string_view key, float value) {
  map[std::string{key}] = ConfigValue{value};
}

void config_set_string(std::string_view key, std::string value) {
  map[std::string{key}] = ConfigValue{value};
}

template <class T>
std::optional<T> config_get(std::string_view key) {
  if (auto v = map.find(key); v != map.end() && std::holds_alternative<T>(v->second)) {
    return std::get<T>(v->second);
  } else {
    return std::nullopt;
  }
}

std::optional<i32> config_get_i32(std::string_view key) { return config_get<i32>(key); }
std::optional<float> config_get_float(std::string_view key) { return config_get<float>(key); }
std::optional<std::string> config_get_string(std::string_view key) { return config_get<std::string>(key); }

void config_get_i32_if_set(std::string_view key, i32& out) {
  auto value = config_get<i32>(key);
  if (value.has_value()) { out = *value; }
}
void config_get_float_if_set(std::string_view key, float& out) {
  auto value = config_get<float>(key);
  if (value.has_value()) { out = *value; }
}
void config_get_string_if_set(std::string_view key, std::string& out) {
  auto value = config_get<std::string>(key);
  if (value.has_value()) { out = *value; }
}

void config_save_to_file(std::string file) {
  std::ofstream stream{file};
  for (auto& v : map) {
    std::stringstream ss;
    ss << std::fixed;
    ss << v.first << ": ";
    std::visit([&](auto&& x) { ss << x; }, v.second);
    ss << "\n";
    std::string str = ss.str();
    stream.write(str.c_str(), str.size());
  }
}
void config_load_from_file(std::string file) {
  std::ifstream stream{file};
  if (!stream.is_open()) {
    std::ofstream ofstream{file};
    stream.open(file);
  }
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string config = ss.str();

  std::string key{};
  std::string value_str{};
  bool insert_to_map = false;
  std::string* src = &key;

  auto insert_ = [](std::string_view key_, std::string value_str_) {
    try {
      if (value_str_.find('.') != std::string::npos) {
        float value = std::stof(value_str_, nullptr);
        config_set_float(key_, value);
      } else {
        i32 value = std::stoi(value_str_, nullptr);
        config_set_i32(key_, value);
      }
    } catch (std::exception) {
      config_set_string(key_, value_str_);
    }
  };

  for (size_t i = 0; i < config.size(); i += 1) {

    if (config[i] == ':' && src == &key) {
      i += 1;
      src = &value_str;
      continue;
    } else if (config[i] == '\r' || config[i] == '\n') {
      src = &key;
      insert_to_map = true;
      continue;
    } else if (insert_to_map) {
      insert_(key, value_str);

      insert_to_map = false;
      key = "";
      value_str = "";
    }

    (*src) += config[i];
  }

  insert_(key, value_str);
}
