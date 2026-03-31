#pragma once
#include <functional>
#include <unordered_map>

template <typename... Args>
class Signal final {
  public:
    using slot_key = size_t;
    using slot_t = std::function<void(Args...)>;

    mutable std::unordered_map<slot_key, slot_t> slots;
    mutable slot_key current_key = 0;

    slot_key connect(slot_t slot) const {
      slots[current_key] = slot;
      return current_key++;
    }

    void disconnect(slot_key key) const {
      slots.erase(key);
    }

    void emit(Args... args) const {
      for (const auto& slot : slots) {
        slot.second(args...);
      }
    }
};
