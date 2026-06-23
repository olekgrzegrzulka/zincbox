#include <fstream>
#include <optional>
#include <string>
#include "common/logger.hpp"
#include "config.hpp"
#include "core/io.hpp"
#include "lib/json.cpp/json.h"

jt::Json json_;

void config::set_i32(const std::string& key, i32 value) { json_[key] = value; }
void config::set_float(const std::string& key, float value) { json_[key] = value; }
void config::set_string(const std::string& key, std::string value) { json_[key] = std::move(value); }

std::optional<i32> config::get_i32(const std::string& key) {
  return json_[key].isNumber() ? std::make_optional(json_[key].getNumber()) : std::nullopt;
}
std::optional<float> config::get_float(const std::string& key) {
  return json_[key].isDouble() ? std::make_optional(json_[key].getDouble()) : std::nullopt;
}
std::optional<std::string> config::get_string(const std::string& key) {
  return json_[key].isString() ? std::make_optional(json_[key].getString()) : std::nullopt;
}

void config::save_to_file() {
  std::ofstream stream{io::get_cfg_path()};
  stream << json_.toStringPretty();
  stream.flush();
  stream.close();
}

void config::load_from_file() {
  namespace fs = std::filesystem;
  fs::path cfg_path = io::get_cfg_path();

  auto create_empty_config = [&]() {
    std::ofstream{cfg_path} << "{}";
    return jt::Json::parse("{}").second;
  };

  if (!fs::exists(cfg_path)) {
    json_ = create_empty_config();
    return;
  }

  std::ifstream stream{cfg_path};
  std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

  auto [status, parsed_data] = jt::Json::parse(content);

  if (status != jt::Json::Status::success) {
    out::log_error("failed to load config file ({})", jt::Json::StatusToString(status));

    std::error_code ec;
    fs::rename(cfg_path, cfg_path.string() + ".bak", ec);

    if (ec) { out::log_error("failed to create backup: {}", ec.message()); }

    json_ = create_empty_config();
    return;
  }

  json_ = std::move(parsed_data);
}

jt::Json& config::json() { return json_; }
