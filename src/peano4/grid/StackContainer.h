// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "StackTopology.h"
#include "tarch/Assertions.h"

#include <algorithm>
#include <array>
#include <set>
#include <utility>
#include <vector>

namespace peano4::grid {
  template <class StackType>
  class StackContainer;
} // namespace peano4::grid

template <class StackType>
class peano4::grid::StackContainer {
public:
  using Stack                 = StackType;
  using ParallelExchangeStack = StackTopology::ParallelExchangeStack;

  static constexpr auto LocalStackCount       = StackTopology::TotalNumberOfLocalExchangeStacks;
  static constexpr auto ParallelStacksPerSlot = static_cast<int>(ParallelExchangeStack::NumberOfStacks);

  using LocalStacksPerTree       = std::array<Stack, static_cast<std::size_t>(LocalStackCount)>;
  using ParallelStacksForOneSlot = std::array<Stack, static_cast<std::size_t>(ParallelStacksPerSlot)>;

  void setLocalTrees(const std::set<int>& localTreeIds) {
    std::vector<PerTreeStorage>      newTrees(localTreeIds.size());
    std::vector<std::pair<int, int>> newMap;
    newMap.reserve(localTreeIds.size());

    int idx = 0;
    for (int treeId : localTreeIds) {
      newMap.emplace_back(treeId, idx);

      const int oldIndex = tryGetLocalTreeIndex(treeId);
      if (oldIndex >= 0) {
        newTrees[idx] = std::move(_trees[oldIndex]);
      }
      idx++;
    }

    _trees              = std::move(newTrees);
    _treeIdToLocalIndex = std::move(newMap);
  }

  Stack& getLocalExchangeStack(int treeId, int localStackId) {
    assertion(localStackId >= 0);
    assertion(localStackId < LocalStackCount);

    int localIndex = getLocalTreeIndex(treeId);
    return _trees[localIndex].local[localStackId];
  }

  // TODO: Should be resized when we set the local trees?
  void resizeParallelExchangeStacks(int treeId, int numberOfSlots) {
    assertion(numberOfSlots >= 0);
    const int localIndex = getLocalTreeIndex(treeId);
    if (_trees[localIndex].parallel.size() < static_cast<std::size_t>(numberOfSlots)) {
      _trees[localIndex].parallel.resize(static_cast<std::size_t>(numberOfSlots));
    }
  }

  Stack& getParallelExchangeStack(int treeId, int slot, ParallelExchangeStack which) {
    assertion(slot >= 0);

    const int localIndex = getLocalTreeIndex(treeId);

    assertion(static_cast<std::size_t>(slot) < _trees[localIndex].parallel.size());

    const int whichIndex = StackTopology::toIndex(which);
    assertion(whichIndex >= 0);
    assertion(whichIndex < ParallelStacksPerSlot);

    return _trees[localIndex].parallel[static_cast<std::size_t>(slot)][static_cast<std::size_t>(whichIndex)];
  }

  std::size_t size() const {
    std::size_t count = 0;
    for (const auto& tree : _trees) {
      for (const auto& stack : tree.local) {
        count += stack.size();
      }
      for (const auto& neighborSlot : tree.parallel) {
        for (const auto& stack : neighborSlot) {
          count += stack.size();
        }
      }
    }
    return count;
  }

  void clear() {
    _trees.clear();
    _treeIdToLocalIndex.clear();
  }

private:
  struct PerTreeStorage {
    LocalStacksPerTree                    local;
    std::vector<ParallelStacksForOneSlot> parallel;
  };

  int tryGetLocalTreeIndex(int treeId) const {
    auto it = std::lower_bound(
      _treeIdToLocalIndex.begin(),
      _treeIdToLocalIndex.end(),
      treeId,
      [](const std::pair<int, int>& p, int v) { return p.first < v; }
    );
    if (it == _treeIdToLocalIndex.end() or it->first != treeId) {
      return -1;
    }
    return it->second;
  }

  int getLocalTreeIndex(int treeId) const {
    const int idx = tryGetLocalTreeIndex(treeId);
    assertion(idx >= 0, "Tree id not registered");
    return idx;
  }

  std::vector<PerTreeStorage> _trees;
  // Sorted by tree id.
  std::vector<std::pair<int, int>> _treeIdToLocalIndex;
};
