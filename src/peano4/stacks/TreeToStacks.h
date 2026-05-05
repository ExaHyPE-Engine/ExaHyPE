// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/parallel/Node.h"

#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace peano4::stacks {
  template <class T>
  class TreeToStacks;

  /**
   * Unique key identifying a stack
   *
   * To look up the right stack, we use a combination of tree number (id) and
   * stack number. The tree number has to be positive, but it might refer to
   * a local or a remote tree. In the latter case, the identified stack is a
   * data exchange stack. The second entry is always positive and it identifies
   * which stack of the corresponding tree is meant. The class
   * peano4::grid::PeanoCurve contains information on pre-defined stack
   * numbers.
   */
  using StackKey = std::pair<int, int>; // (tree number, stack number)
} // namespace peano4::stacks

template <class T>
class peano4::stacks::TreeToStacks {
private:
  /**
   * Data of one tree
   */
  struct /*alignas(64)*/ TreeData {
    std::unordered_map<int, std::unique_ptr<T>> stacks;
    // mutable std::shared_mutex _mutex;
  };

  // Each element represents the data of one tree.
  // However, we do not assume that the index of each entry is a local tree id.
  // We use global ids instead to avoid the overhead in the computation to the local id.
  // Has to be a std::vector to allow lock-free concurrent access.
  // Requires resize() to be called before usage of trees.
  std::vector<TreeData> _trees;

  std::size_t _stackSizeHint;

  INLINE_METHOD T* createStack(int treeId, int stackId) {
    assertion3(treeId >= 0, treeId, stackId, typeid(T).name());
    assertion3(stackId >= 0, treeId, stackId, typeid(T).name());
    assertion3(treeId < static_cast<int>(_trees.size()), treeId, stackId, typeid(T).name());
    assertion3(_trees[treeId].stacks.count(stackId) <= 1, treeId, stackId, typeid(T).name());
    auto& stacks        = _trees[treeId].stacks;
    auto [it, inserted] = stacks.try_emplace(stackId);
    if (inserted) {
      it->second = std::make_unique<T>();
      it->second->reserve(_stackSizeHint);
      logDebug(" Created stack " << stackId << " in tree " << treeId << " for " << typeid(T).name());
    }
    return it->second.get();
  }

public:
  INLINE_METHOD explicit TreeToStacks() noexcept:
    _trees(parallel::Node::MaxSpacetreesPerRank),
    _stackSizeHint(0) {}

  ~TreeToStacks() noexcept = default;

  // Returns true if no stack exists for the given tree/stack id or if that stack is empty.
  INLINE_METHOD bool empty(int treeId, int stackId) const {
    assertion3(treeId >= 0, treeId, stackId, typeid(T).name());
    assertion3(stackId >= 0, treeId, stackId, typeid(T).name());
    assertion4(treeId < static_cast<int>(_trees.size()), _trees.size(), treeId, stackId, typeid(T).name());

    // std::shared_lock<std::shared_mutex> readLock(_trees[treeId]._mutex);
    auto it = _trees[treeId].stacks.find(stackId);
    return (it == _trees[treeId].stacks.end() or it->second->empty());
  }

  INLINE_METHOD void resize(std::size_t size) { _trees.resize(size); }

  // Returns the total number of elements stored across all existing stacks.
  INLINE_METHOD std::size_t size() const {
    std::size_t total = 0;
    for (const auto& tree : _trees) {
      // std::shared_lock<std::shared_mutex> readLock(tree._mutex);
      for (const auto& treeData : tree.stacks) {
        if (treeData.second) {
          total += treeData.second->size();
        }
      }
    }
    return total;
  }

  INLINE_METHOD void setStackSizeHint(const std::size_t stackSizeHint) { _stackSizeHint = stackSizeHint; }

  // Clears all stacks for all spacetrees.
  INLINE_METHOD void clear() {
    for (auto& tree : _trees) {
      // std::unique_lock<std::shared_mutex> writeLock(tree._mutex);
      for (auto& treeData : tree.stacks) {
        treeData.second->clear();
        // Don't do the reset, it is dangerous!
        // TODO: Document why
        // treeData.second.reset(new T());
      }
    }
  }

  // Clears all stacks for the given spacetree.
  INLINE_METHOD void clear(int treeId) {
    assertion2(treeId >= 0, treeId, typeid(T).name());
    if (treeId < static_cast<int>(_trees.size())) {
      // std::unique_lock<std::shared_mutex> writeLock(_trees[treeId]._mutex);
      for (auto& treeData : _trees[treeId].stacks) {
        treeData.second->clear();
        // Don't do the reset, it is dangerous!
        // treeData.second.reset(new T());
      }
    }
  }

  // Lazy accessor: returns the stack for push; creates it if missing.
  INLINE_METHOD T* getForPush(const StackKey& key) {
    const int treeId  = key.first;
    const int stackId = key.second;
    assertion3(treeId >= 0, treeId, stackId, typeid(T).name());
    assertion3(stackId >= 0, treeId, stackId, typeid(T).name());
    assertion4(treeId < static_cast<int>(_trees.size()), _trees.size(), treeId, stackId, typeid(T).name());
    return createStack(treeId, stackId);

    // {
    //   std::shared_lock<std::shared_mutex> readLock(_trees[treeId]._mutex);
    //   auto it = _trees[treeId].stacks.find(stackId);
    //   if (it != _trees[treeId].stacks.end()) {
    //     return it->second.get();
    //   }
    // }

    // {
    //   std::unique_lock<std::shared_mutex> writeLock(_trees[treeId]._mutex);
    //   auto it = _trees[treeId].stacks.find(stackId); // Check again
    //   if (it != _trees[treeId].stacks.end()) {
    //     return it->second.get();
    //   }
    //   createStack(treeId, stackId);
    //   return _trees[treeId].stacks[stackId].get();
    // }
  }
  INLINE_METHOD T* getForPush(int treeId, int stackId) { return getForPush(StackKey(treeId, stackId)); }

  // Returns the stack for pop; it must already exist.
  INLINE_METHOD T* getForPop(const StackKey& key) {
    const int treeId  = key.first;
    const int stackId = key.second;
    assertion3(treeId >= 0, treeId, stackId, typeid(T).name());
    assertion3(stackId >= 0, treeId, stackId, typeid(T).name());
    assertion4(treeId < static_cast<int>(_trees.size()), _trees.size(), treeId, stackId, typeid(T).name());

    // std::shared_lock<std::shared_mutex> readLock(_trees[treeId]._mutex);
    auto it = _trees[treeId].stacks.find(stackId);
    assertion3(it != _trees[treeId].stacks.end(), treeId, stackId, typeid(T).name());
    return it->second.get();
  }
  INLINE_METHOD T* getForPop(int treeId, int stackId) { return getForPop(StackKey(treeId, stackId)); }

  std::string toString() const {
    std::size_t totalStacks = 0;
    for (const auto& tree : _trees) {
      totalStacks += tree.stacks.size(); // Number of (treeId, stackId) entries
    }
    std::ostringstream oss;
    oss << "TreeToStacks{Trees=" << _trees.size() << ", Stacks=" << totalStacks << "}";
    return oss.str();
  }

  // Returns keys for tree id and their stack ids.
  INLINE_METHOD std::vector<StackKey> getKeys() const {
    std::size_t total = 0;
    for (const auto& tree : _trees) {
      total += tree.stacks.size();
    }
    std::vector<StackKey> keys;
    keys.reserve(total);
    for (int i = 0; i < static_cast<int>(_trees.size()); i++) {
      // std::shared_lock<std::shared_mutex> readLock(_trees[i]._mutex);
      for (const auto& treeData : _trees[i].stacks) {
        keys.emplace_back(StackKey(i, treeData.first));
      }
    }
    return keys;
  }

  // Returns keys only for the specified tree id.
  // Thread-safe relative to other trees if creation/deletion is restricted to local scope.
  INLINE_METHOD std::vector<StackKey> getKeys(int treeId) const {
    assertion2(treeId >= 0, treeId, typeid(T).name());
    if (treeId >= static_cast<int>(_trees.size())) {
      return {};
    }

    // std::shared_lock<std::shared_mutex> lock(_trees[treeId]._mutex);
    std::vector<StackKey> keys;
    const auto&           treeData = _trees[treeId].stacks;
    keys.reserve(treeData.size());

    for (const auto& pair : treeData) {
      keys.emplace_back(StackKey(treeId, pair.first));
    }
    return keys;
  }

  // Performs garbage collection on the stacks of a given spacetree.
  INLINE_METHOD void garbageCollection(int treeId) {
    assertion2(treeId >= 0, treeId, typeid(T).name());
    if (treeId >= static_cast<int>(_trees.size())) {
      return;
    }

    // std::unique_lock<std::shared_mutex> writeLock(_trees[treeId]._mutex);
    auto& treeData = _trees[treeId].stacks;

    for (auto it = treeData.begin(); it != treeData.end(); /* no ++ here */) {
      const int  stackId   = it->first;
      const bool isStorage = parallel::Node::isStorageStackNumber(stackId); // TODO: Should not be in Node?
      const bool isEmpty   = (not it->second or it->second->empty());
      if (not isStorage and isEmpty) {
        it = treeData.erase(it); // Remove empty non-storage stack
      } else {
        ++it;
      }
    }
  }

  // Returns true if a stack exists for the given tree/stack id.
  INLINE_METHOD bool holdsStack(const StackKey& key) const {
    const int treeId  = key.first;
    const int stackId = key.second;
    assertion3(treeId >= 0, treeId, stackId, typeid(T).name());
    assertion3(stackId >= 0, treeId, stackId, typeid(T).name());
    assertion4(treeId < static_cast<int>(_trees.size()), _trees.size(), treeId, stackId, typeid(T).name());
    // std::shared_lock<std::shared_mutex> readLock(_trees[treeId]._mutex);
    return _trees[treeId].stacks.count(stackId) == 1;
  }
  INLINE_METHOD bool holdsStack(int treeId, int stackId) const { return holdsStack(StackKey(treeId, stackId)); }
};
