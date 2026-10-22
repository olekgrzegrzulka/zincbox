#pragma once
#include <functional>
#include <unordered_map>

template <typename... Args> class Signal final {
  public:
    using slot_key = size_t;
    using slot_t = std::function<void(Args...)>;

  private:
    mutable std::unordered_map<slot_key, slot_t> m_slots;
    mutable slot_key m_current_key = 0;

  public:
    slot_key connect(slot_t slot) const {
      m_slots[m_current_key] = slot;
      return m_current_key++;
    }

    void disconnect(slot_key key) const { m_slots.erase(key); }

    void emit(Args... args) const {
      for (const auto& slot : m_slots) {
        slot.second(args...);
      }
    }
};
