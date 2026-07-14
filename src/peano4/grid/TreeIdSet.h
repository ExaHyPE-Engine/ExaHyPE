// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::grid {
  class TreeIdSet {
  public:
    using value_type     = int;
    using iterator       = std::vector<int>::iterator;
    using const_iterator = std::vector<int>::const_iterator;

    TreeIdSet() = default;

    TreeIdSet(std::initializer_list<int> init) { insert(init.begin(), init.end()); }

    bool empty() const noexcept { return _data.empty(); }

    std::size_t size() const noexcept { return _data.size(); }

    iterator begin() noexcept { return _data.begin(); }

    iterator end() noexcept { return _data.end(); }

    const_iterator begin() const noexcept { return _data.begin(); }

    const_iterator end() const noexcept { return _data.end(); }

    const_iterator cbegin() const noexcept { return _data.cbegin(); }

    const_iterator cend() const noexcept { return _data.cend(); }

    std::size_t count(const int& val) const noexcept {
      for (const int current : _data) {
        if (current == val) {
          return 1;
        }
      }
      return 0;
    }

    void clear() noexcept { _data.clear(); }

    void insert(const int& val) {
      if (count(val) == 1) {
        return;
      }

      if (_data.empty() and _data.capacity() == 0) {
        _data.reserve(16);
      }

      _data.push_back(val);
    }

    template <typename InputIt>
    void insert(InputIt first, InputIt last) {
      for (; first != last; ++first) {
        insert(*first);
      }
    }

    void erase(const int& val) {
      for (std::size_t i = 0; i < _data.size(); ++i) {
        if (_data[i] == val) {
          _data[i] = _data.back();
          _data.pop_back();
          return;
        }
      }
    }

  private:
    std::vector<int> _data;
  };
} // namespace peano4::grid
