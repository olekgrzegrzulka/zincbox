#pragma once
#include <filesystem>
#include <future>
#include <vector>
#include <nfd.hpp>
#include "common/utf.hpp"

namespace zincbox::ui::file_dialog {

  template <typename T> bool is_ready(const std::future<T>& f) {
    using namespace std::chrono_literals;
    return f.valid() && f.wait_for(0s) == std::future_status::ready;
  }

  std::future<std::vector<std::filesystem::path>> pick_folder_multiple() {
    return std::async(std::launch::async, []() -> std::vector<std::filesystem::path> {
      std::vector<std::filesystem::path> paths;
      NFD::UniquePathSet out_paths;
      auto result = NFD::PickFolderMultiple(out_paths, (const nfdu8char_t*)nullptr);
      if (result == NFD_OKAY) {
        nfdpathsetsize_t numPaths;
        NFD::PathSet::Count(out_paths, numPaths);
        if (numPaths > 0) {
          nfdpathsetsize_t i;
          for (i = 0; i < numPaths; i += 1) {
            NFD::UniquePathSetPathU8 path_utf8;
            NFD::PathSet::GetPath(out_paths, i, path_utf8);
            paths.emplace_back(utf8_to_path(path_utf8.get()));
          }
        }
      }
      return paths;
    });
  }

  std::future<std::vector<std::filesystem::path>> pick_folder() {
    return std::async(std::launch::async, []() -> std::vector<std::filesystem::path> {
      NFD::UniquePathU8 out_path;
      if (NFD::PickFolder(out_path, (const nfdu8char_t*)nullptr) == NFD_OKAY) { return {utf8_to_path(out_path.get())}; }
      return {};
    });
  }

  std::future<std::vector<std::filesystem::path>> save_json_dialog(std::string default_name, std::string filter_label) {
    return std::async(std::launch::async,
                      [default_name = std::move(default_name),
                       filter_label = std::move(filter_label)]() -> std::vector<std::filesystem::path> {
                        NFD::UniquePathU8 outPath;
                        nfdfilteritem_t filterList[1] = {{filter_label.c_str(), "json"}};
                        if (NFD::SaveDialog(outPath, filterList, 1, nullptr, default_name.c_str()) == NFD_OKAY) {
                          return {utf8_to_path(outPath.get())};
                        }
                        return {};
                      });
  }

  std::future<std::vector<std::filesystem::path>> pick_image_dialog(std::string filter_label) {
    return std::async(std::launch::async,
                      [filter_label = std::move(filter_label)]() -> std::vector<std::filesystem::path> {
                        NFD::UniquePathU8 out_path_n;
                        nfdfilteritem_t filter_item[1] = {{filter_label.c_str(), "png,jpg,jpeg"}};
                        if (NFD::OpenDialog(out_path_n, filter_item, 1) == NFD_OKAY) {
                          return {utf8_to_path(out_path_n.get())};
                        }
                        return {};
                      });
  }

}; // namespace zincbox::ui::file_dialog
