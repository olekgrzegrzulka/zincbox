#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
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

    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::TextInput* text_input() noexcept { return m_text_input; }
    [[nodiscard]] const zincgui::TextInput* text_input() const noexcept { return m_text_input; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_cancel() noexcept { return m_btn_cancel; }
    [[nodiscard]] const zincgui::Button* btn_cancel() const noexcept { return m_btn_cancel; }
    [[nodiscard]] zincgui::Button* btn_ok() noexcept { return m_btn_ok; }
    [[nodiscard]] const zincgui::Button* btn_ok() const noexcept { return m_btn_ok; }

    PopupInput& on_ok_pressed(std::function<void()> fn) {
      m_on_ok_pressed = std::move(fn);
      return *this;
    }
    PopupInput& on_cancel_pressed(std::function<void()> fn) {
      m_on_cancel_pressed = std::move(fn);
      return *this;
    }

  private:
    zincgui::Label* m_title{};
    zincgui::TextInput* m_text_input{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_cancel{};
    zincgui::Button* m_btn_ok{};
    std::function<void()> m_on_ok_pressed;
    std::function<void()> m_on_cancel_pressed;
};

class PopupConfirm : public Popup {
  public:
    PopupConfirm(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                 std::string_view content_);

    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::Label* content() noexcept { return m_content; }
    [[nodiscard]] const zincgui::Label* content() const noexcept { return m_content; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_cancel() noexcept { return m_btn_cancel; }
    [[nodiscard]] const zincgui::Button* btn_cancel() const noexcept { return m_btn_cancel; }
    [[nodiscard]] zincgui::Button* btn_ok() noexcept { return m_btn_ok; }
    [[nodiscard]] const zincgui::Button* btn_ok() const noexcept { return m_btn_ok; }

    PopupConfirm& on_ok_pressed(std::function<void()> fn) {
      m_on_ok_pressed = std::move(fn);
      return *this;
    }
    PopupConfirm& on_cancel_pressed(std::function<void()> fn) {
      m_on_cancel_pressed = std::move(fn);
      return *this;
    }

  private:
    zincgui::Label* m_title{};
    zincgui::Label* m_content{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_cancel{};
    zincgui::Button* m_btn_ok{};
    std::function<void()> m_on_ok_pressed;
    std::function<void()> m_on_cancel_pressed;
};

class PopupImportFolders final : public Popup {
  public:
    PopupImportFolders(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::span<const std::filesystem::path> dropped_directories);

    [[nodiscard]] const std::vector<std::filesystem::path>& dirs() const noexcept { return m_dirs; }
    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::Label* content() noexcept { return m_content; }
    [[nodiscard]] const zincgui::Label* content() const noexcept { return m_content; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_cancel() noexcept { return m_btn_cancel; }
    [[nodiscard]] const zincgui::Button* btn_cancel() const noexcept { return m_btn_cancel; }
    [[nodiscard]] zincgui::Button* btn_add_collections() noexcept { return m_btn_add_collections; }
    [[nodiscard]] const zincgui::Button* btn_add_collections() const noexcept { return m_btn_add_collections; }
    [[nodiscard]] zincgui::Button* btn_merge() noexcept { return m_btn_merge; }
    [[nodiscard]] const zincgui::Button* btn_merge() const noexcept { return m_btn_merge; }

    PopupImportFolders& on_add_collections_pressed(std::function<void(std::span<const std::filesystem::path>)> fn) {
      m_on_add_collections_pressed = std::move(fn);
      return *this;
    }
    PopupImportFolders& on_merge_pressed(std::function<void(std::span<const std::filesystem::path>)> fn) {
      m_on_merge_pressed = std::move(fn);
      return *this;
    }

  private:
    std::vector<std::filesystem::path> m_dirs;
    zincgui::Label* m_title{};
    zincgui::Label* m_content{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_cancel{};
    zincgui::Button* m_btn_add_collections{};
    zincgui::Button* m_btn_merge{};
    std::function<void(std::span<const std::filesystem::path>)> m_on_add_collections_pressed;
    std::function<void(std::span<const std::filesystem::path>)> m_on_merge_pressed;
};

class PopupSetSources final : public Popup {
  public:
    PopupSetSources(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                    size_t collection_id_);

    void update() override;

    [[nodiscard]] size_t collection_id() const noexcept { return m_collection_id; }
    PopupSetSources& collection_id(size_t id) noexcept {
      m_collection_id = id;
      return *this;
    }

    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] ScrollableView* scrollable_content() noexcept { return m_scrollable_content; }
    [[nodiscard]] const ScrollableView* scrollable_content() const noexcept { return m_scrollable_content; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_close() noexcept { return m_btn_close; }
    [[nodiscard]] const zincgui::Button* btn_close() const noexcept { return m_btn_close; }
    [[nodiscard]] zincgui::Button* btn_add_dir() noexcept { return m_btn_add_dir; }
    [[nodiscard]] const zincgui::Button* btn_add_dir() const noexcept { return m_btn_add_dir; }

    PopupSetSources& on_add_dir_pressed(std::function<void()> fn) {
      m_on_add_dir_pressed = std::move(fn);
      return *this;
    }
    PopupSetSources& on_remove_path_pressed(std::function<void(const std::filesystem::path&)> fn) {
      m_on_remove_path_pressed = std::move(fn);
      return *this;
    }

  private:
    size_t m_collection_id;
    zincgui::Label* m_title{};
    ScrollableView* m_scrollable_content{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_close{};
    zincgui::Button* m_btn_add_dir{};
    double m_scroll_px{};
    double m_target_scroll_px{};
    i32 m_max_path_label_width = 0;
    std::function<void()> m_on_add_dir_pressed;
    std::function<void(const std::filesystem::path&)> m_on_remove_path_pressed;
};

class PopupAddToPlaylist final : public Popup {
  public:
    PopupAddToPlaylist(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::optional<size_t> track_id_);

    void update() override;

    [[nodiscard]] const std::optional<size_t>& track_id() const noexcept { return m_track_id; }
    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::Label* content() noexcept { return m_content; }
    [[nodiscard]] const zincgui::Label* content() const noexcept { return m_content; }
    [[nodiscard]] PanelPlaylists* playlists_view() noexcept { return m_playlists_view; }
    [[nodiscard]] const PanelPlaylists* playlists_view() const noexcept { return m_playlists_view; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_cancel() noexcept { return m_btn_cancel; }
    [[nodiscard]] const zincgui::Button* btn_cancel() const noexcept { return m_btn_cancel; }

    PopupAddToPlaylist& on_playlist_selected(std::function<void(size_t)> fn) {
      m_on_playlist_selected = std::move(fn);
      return *this;
    }

  private:
    std::optional<size_t> m_track_id;
    zincgui::Label* m_title{};
    zincgui::Label* m_content{};
    PanelPlaylists* m_playlists_view{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_cancel{};
    std::function<void(size_t)> m_on_playlist_selected;
};

class PopupCreateSmartPlaylist final : public Popup {
  public:
    PopupCreateSmartPlaylist(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_cancel() noexcept { return m_btn_cancel; }
    [[nodiscard]] const zincgui::Button* btn_cancel() const noexcept { return m_btn_cancel; }
    [[nodiscard]] zincgui::Button* btn_add() noexcept { return m_btn_add; }
    [[nodiscard]] const zincgui::Button* btn_add() const noexcept { return m_btn_add; }
    [[nodiscard]] zincgui::Label* label_artists() noexcept { return m_label_artists; }
    [[nodiscard]] const zincgui::Label* label_artists() const noexcept { return m_label_artists; }
    [[nodiscard]] zincgui::TextInput* input_artists() noexcept { return m_input_artists; }
    [[nodiscard]] const zincgui::TextInput* input_artists() const noexcept { return m_input_artists; }
    [[nodiscard]] zincgui::Label* label_name() noexcept { return m_label_name; }
    [[nodiscard]] const zincgui::Label* label_name() const noexcept { return m_label_name; }
    [[nodiscard]] zincgui::TextInput* input_name() noexcept { return m_input_name; }
    [[nodiscard]] const zincgui::TextInput* input_name() const noexcept { return m_input_name; }

    PopupCreateSmartPlaylist& on_cancel(std::function<void()> fn) {
      m_on_cancel = std::move(fn);
      return *this;
    }
    PopupCreateSmartPlaylist& on_add(std::function<void()> fn) {
      m_on_add = std::move(fn);
      return *this;
    }

  private:
    zincgui::Label* m_title{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_cancel{};
    zincgui::Button* m_btn_add{};
    zincgui::Label* m_label_artists{};
    zincgui::TextInput* m_input_artists{};
    zincgui::Label* m_label_name{};
    zincgui::TextInput* m_input_name{};
    std::function<void()> m_on_cancel;
    std::function<void()> m_on_add;
    double m_scroll_px{};
    double m_target_scroll_px{};
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

    [[nodiscard]] zincgui::Label* title() noexcept { return m_title; }
    [[nodiscard]] const zincgui::Label* title() const noexcept { return m_title; }
    [[nodiscard]] zincgui::Label* header() noexcept { return m_header; }
    [[nodiscard]] const zincgui::Label* header() const noexcept { return m_header; }
    [[nodiscard]] ScrollableView* scroll_view() noexcept { return m_scroll_view; }
    [[nodiscard]] const ScrollableView* scroll_view() const noexcept { return m_scroll_view; }
    [[nodiscard]] zincgui::Widget* buttons() noexcept { return m_buttons; }
    [[nodiscard]] const zincgui::Widget* buttons() const noexcept { return m_buttons; }
    [[nodiscard]] zincgui::Button* btn_ok() noexcept { return m_btn_ok; }
    [[nodiscard]] const zincgui::Button* btn_ok() const noexcept { return m_btn_ok; }

    PopupScanSummary& on_ok_pressed(std::function<void()> fn) {
      m_on_ok_pressed = std::move(fn);
      return *this;
    }

  private:
    std::vector<std::vector<db::track_info>> m_display_tracks_storage;
    zincgui::Label* m_title{};
    zincgui::Label* m_header{};
    ScrollableView* m_scroll_view{};
    zincgui::Widget* m_buttons{};
    zincgui::Button* m_btn_ok{};
    std::function<void()> m_on_ok_pressed;
};
