#include "cover_art.hpp"
#include <cstdio>
#include <filesystem>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "common/logger.hpp"
#include "common/utf.hpp"
#include "core/cover_art.hpp"
#include "core/musicdb/types.hpp"
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
  atlas = zincgui::TextureAtlas{2048, 4, 4};
  auto res = theme::get_raw_resource("cover_unknown.png");
  if (res.empty()) { out::critical("failed to load cover_unknown.png, empty"); }

  int width, height, channels;
  stbi_uc* data = stbi_load_from_memory((stbi_uc*)res.data(), res.size(), &width, &height, &channels, STBI_rgb_alpha);

  if (!data) { out::critical("failed to load cover_unknown.png"); }

  atlas.add_texture("cover_unknown", (u8*)data, width, height);
  atlas.set_fallback_texture("cover_unknown");
}

void zincbox::ui::cover_art::bind() { atlas.bind(2); }

void clean() {
  for (auto&& id : lru.evict(16)) {
    atlas.remove_texture(id);
  }
}

static std::unordered_set<db::track_id_t> pending_tracks;
static constexpr i32 MAX_BATCH_SIZE = 16;

std::optional<std::string> zincbox::ui::cover_art::get_texture_id(db::track_id_t track_id) {
  auto path = zincbox::cover_art::get_64(track_id);
  if (!path) {
    pending_tracks.insert(track_id);
    return std::nullopt;
  }

  std::string id = path_to_utf8(path->stem());
  lru.touch(id);

  if (atlas.has_texture(id, 1)) { return id; }

  pending_tracks.insert(track_id);
  return std::nullopt;
}

bool zincbox::ui::cover_art::update() {
  if (pending_tracks.empty()) { return false; }

  bool modified = false;
  i32 processed = 0;

  auto it = pending_tracks.begin();
  while (it != pending_tracks.end() && processed < MAX_BATCH_SIZE) {
    db::track_id_t track_id = *it;
    it = pending_tracks.erase(it);
    processed += 1;
    auto path = zincbox::cover_art::get_64(track_id);
    if (!path) { continue; }
    std::string id = path_to_utf8(path->stem());
    if (atlas.has_texture(id, 1)) { continue; }
    if (!atlas.can_fit_texture(64, 64)) {
      clean();
      modified = true;
    }
    if (!atlas.can_fit_texture(64, 64)) {
      pending_tracks.insert(track_id);
      break;
    }
    atlas.add_texture(id, *path);
    modified = true;
  }
  return modified;
}
