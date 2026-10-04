#include "cover_art.hpp"
#include <cstdio>
#include <filesystem>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/utf.hpp"
#include "core/cover_art.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "lib/stb_image/stb_image.h"
#include "ui/theme.hpp"
#include "ui/zincgui/texture_atlas.hpp"

class LRU {
  private:
    std::list<std::string> order;
    std::unordered_map<std::string, std::list<std::string>::iterator> lookup;

  public:
    void touch(const std::string& id) {
      auto it = lookup.find(id);
      if (it != lookup.end()) {
        order.splice(order.begin(), order, it->second);
      } else {
        order.push_front(id);
        lookup[id] = order.begin();
      }
    }

    size_t size() const { return lookup.size(); }

    std::vector<std::string> evict(size_t n) {
      std::vector<std::string> evicted;
      while (n > 0 && !order.empty()) {
        std::string id = std::move(order.back());
        order.pop_back();
        lookup.erase(id);
        evicted.push_back(std::move(id));
        n -= 1;
      }
      return evicted;
    }
} lru;

static zincgui::TextureAtlas atlas;

zincgui::TextureAtlas& zincbox::ui::cover_art::texture_atlas() { return atlas; }

void zincbox::ui::cover_art::init() {
  atlas = zincgui::TextureAtlas{4096, 0, 64};
  atlas.set_filter(true);

  std::array<std::string, 2> texture_paths = {"cover_unknown.png", "panel_playlists/button_add_playlist.png"};
  std::array<std::string, 2> textures = {"cover_unknown", "button_add_playlist"};

  for (size_t i : {0, 1}) {
    auto res = theme::get_raw_resource(texture_paths[i]);
    if (res.empty()) { out::critical("failed to load " + textures[i] + ", empty"); }
    int width, height, channels;
    stbi_uc* data = stbi_load_from_memory((stbi_uc*)res.data(), res.size(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) { out::critical("failed to load " + textures[i]); }
    atlas.add_texture(textures[i], (u8*)data, width, height);
  }
  atlas.set_fallback_texture("cover_unknown");
}

void clean() {
  for (auto&& id : lru.evict(16)) {
    atlas.remove_texture(id);
  }
}

struct PendingTracksHash {
    size_t operator()(const std::pair<size_t, bool>& a) const noexcept {
      size_t seed = std::hash<size_t>{}(a.first);
      seed ^= std::hash<bool>{}(a.second) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      return seed;
    }
};

static std::unordered_set<std::pair<db::track_id_t, bool>, PendingTracksHash> pending_tracks;
static constexpr i32 MAX_BATCH_SIZE = 16;

std::optional<std::string> zincbox::ui::cover_art::get_texture_id(db::track_id_t track_id, bool high_res) {
  auto path = high_res ? zincbox::cover_art::get_256(track_id) : zincbox::cover_art::get_64(track_id);
  if (!path) {
    pending_tracks.insert({track_id, high_res});
    return std::nullopt;
  }

  std::string id = path_to_utf8(path->stem());
  lru.touch(id);

  if (atlas.has_texture(id, 1)) { return id; }

  pending_tracks.insert({track_id, high_res});
  return std::nullopt;
}

std::optional<std::string> zincbox::ui::cover_art::get_texture_id_of_playlist(db::playlist_id_t playlist_id) {
  auto playlist = db::playlist_by_id(playlist_id);
  ensure(playlist);
  if (playlist->get().get_tracks_count() > 0) {
    auto track_id = playlist->get().track_ids.front();
    auto res = get_texture_id(track_id);
    if (res.has_value()) { return res; }
  }
  return "cover_unknown";
}
std::optional<std::string> zincbox::ui::cover_art::get_texture_id_of_collection(db::collection_id_t collection_id) {
  auto collection = db::collection_by_id(collection_id);
  ensure(collection) if (!collection->get().playlist_ids().empty()) {
    auto playlist = db::playlist_by_id(collection->get().playlist_ids().front());
    ensure(playlist);
    if (playlist->get().get_tracks_count() > 0) {
      auto track_id = playlist->get().track_ids.back();
      auto res = get_texture_id(track_id);
      if (res.has_value()) { return res; }
    }
  }
  return "cover_unknown";
}

bool zincbox::ui::cover_art::update() {
  if (pending_tracks.empty()) { return false; }

  bool modified = false;
  i32 processed = 0;

  auto it = pending_tracks.begin();
  while (it != pending_tracks.end() && processed < MAX_BATCH_SIZE) {
    auto [track_id, high_res] = *it;
    it = pending_tracks.erase(it);
    processed += 1;
    auto path = high_res ? zincbox::cover_art::get_256(track_id) : zincbox::cover_art::get_64(track_id);
    if (!path) { continue; }
    std::string id = path_to_utf8(path->stem());
    if (atlas.has_texture(id, 1)) { continue; }
    if (high_res) {
      if (!atlas.can_fit_texture(256, 256)) {
        clean();
        modified = true;
      }
    } else {
      if (!atlas.can_fit_texture(64, 64)) {
        clean();
        modified = true;
      }
    }

    if (high_res) {
      if (!atlas.can_fit_texture(256, 256)) {
        pending_tracks.insert({track_id, high_res});
        break;
      }
    } else {
      if (!atlas.can_fit_texture(64, 64)) {
        pending_tracks.insert({track_id, high_res});
        break;
      }
    }

    atlas.add_texture(id, *path);
    modified = true;
  }
  return modified;
}
