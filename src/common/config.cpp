#include <fstream>
#include <string>
#include "common/logger.hpp"
#include "config.hpp"
#include "core/io.hpp"
#include "lib/json.cpp/json.h"

jt::Json json_;

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
    out::error("failed to load config file ({})", jt::Json::StatusToString(status));

    std::error_code ec;
    fs::rename(cfg_path, cfg_path.string() + ".bak", ec);

    if (ec) { out::error("failed to create backup: {}", ec.message()); }

    json_ = create_empty_config();
    return;
  }

  json_ = std::move(parsed_data);
}

jt::Json& config::json() { return json_; }
