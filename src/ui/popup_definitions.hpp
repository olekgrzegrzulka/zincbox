#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "ui/popup.hpp"

namespace zincbox::scanner {
  struct ImportSummary;
}

class PanelPlaylists;
class ScrollableView;
class PopupController;

namespace zincgui {
  class Widget;
  class Label;
  class Button;
  class TextInput;
  class ScrollBar;
} // namespace zincgui

class PopupInput : public Popup {
  public:
    PopupInput(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    zincgui::Label* title{};
    zincgui::TextInput* text_input{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_cancel{};
    zincgui::Button* btn_ok{};

    std::function<void()> on_ok_pressed;
    std::function<void()> on_cancel_pressed;
};

class PopupConfirm : public Popup {
  public:
    PopupConfirm(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                 std::string_view content_);

    zincgui::Label* title{};
    zincgui::Label* content{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_cancel{};
    zincgui::Button* btn_ok{};

    std::function<void()> on_ok_pressed;
    std::function<void()> on_cancel_pressed;
};

class PopupImportFolders final : public Popup {
  public:
    PopupImportFolders(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::span<const std::filesystem::path> dropped_directories);

    std::vector<std::filesystem::path> dirs;
    zincgui::Label* title{};
    zincgui::Label* content{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_cancel{};
    zincgui::Button* btn_add_collections{};
    zincgui::Button* btn_merge{};

    std::function<void(std::span<const std::filesystem::path>)> on_add_collections_pressed;
    std::function<void(std::span<const std::filesystem::path>)> on_merge_pressed;
};

class PopupSetSources final : public Popup {
  public:
    PopupSetSources(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                    size_t collection_id_);

    void update() override;

    size_t collection_id;
    zincgui::Label* title{};
    zincgui::Widget* content{};
    zincgui::Widget* scrollable_content{};
    zincgui::ScrollBar* scrollbar{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_close{};
    zincgui::Button* btn_add_dir{};
    double scroll_px{};
    double target_scroll_px{};
    i32 max_path_label_width = 0;

    std::function<void()> on_add_dir_pressed;
    std::function<void(const std::filesystem::path&)> on_remove_path_pressed;
};

class PopupAddToPlaylist final : public Popup {
  public:
    PopupAddToPlaylist(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::optional<size_t> track_id_);

    std::optional<size_t> track_id;
    zincgui::Label* title{};
    zincgui::Label* content{};
    PanelPlaylists* playlists_view{};
    Widget* buttons{};
    zincgui::Button* btn_cancel{};

    std::function<void(size_t)> on_playlist_selected;
};

class PopupCreateSmartPlaylist final : public Popup {
  public:
    PopupCreateSmartPlaylist(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    zincgui::Label* title{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_cancel{};
    zincgui::Button* btn_add{};
    zincgui::Label* label_artists{};
    zincgui::TextInput* input_artists{};
    zincgui::Label* label_name{};
    zincgui::TextInput* input_name{};
    std::function<void()> on_cancel;
    std::function<void()> on_add;
    double scroll_px{};
    double target_scroll_px{};
};

class PopupAbout final : public Popup {
  public:
    PopupAbout(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);
};

class PopupWelcome final : public Popup {
  public:
    PopupWelcome(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);
};

class PopupScanSummary final : public Popup {
  public:
    PopupScanSummary(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                     const zincbox::scanner::ImportSummary& summary);

    void update() override;

    std::vector<std::vector<db::track_info>> display_tracks_storage;
    zincgui::Label* title{};
    zincgui::Label* header{};
    ScrollableView* scroll_view{};
    zincgui::Widget* buttons{};
    zincgui::Button* btn_ok{};

    std::function<void()> on_ok_pressed;
};
