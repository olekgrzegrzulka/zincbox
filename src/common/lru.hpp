#pragma once
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
};
