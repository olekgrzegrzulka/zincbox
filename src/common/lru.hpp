#pragma once
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class LRU {
  private:
    std::list<std::string> m_order;
    std::unordered_map<std::string, std::list<std::string>::iterator> m_lookup;

  public:
    void touch(const std::string& id) {
      auto it = m_lookup.find(id);
      if (it != m_lookup.end()) {
        m_order.splice(m_order.begin(), m_order, it->second);
      } else {
        m_order.push_front(id);
        m_lookup[id] = m_order.begin();
      }
    }

    size_t size() const { return m_lookup.size(); }

    std::vector<std::string> evict(size_t n) {
      std::vector<std::string> evicted;
      while (n > 0 && !m_order.empty()) {
        std::string id = std::move(m_order.back());
        m_order.pop_back();
        m_lookup.erase(id);
        evicted.push_back(std::move(id));
        n -= 1;
      }
      return evicted;
    }
};
