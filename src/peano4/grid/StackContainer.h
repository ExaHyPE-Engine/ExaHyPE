// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "PeriodicTopology.h"
#include "StackTopology.h"
#include "tarch/Assertions.h"

namespace peano4::grid {
  template <class StackType>
  class StackContainer;
} // namespace peano4::grid

template <class StackType>
class peano4::grid::StackContainer final {
public:
  using Stack                 = StackType;
  using ParallelExchangeStack = StackTopology::ParallelExchangeStack;
  using ElementType           = std::decay_t<decltype(std::declval<StackType&>().pop())>;

  void stageCrossRankPeriodicEntry(
    int                  sendTree,
    int                  recvTree,
    const PeriodicStamp& stamp,
    int                  depositStack,
    ElementType          data
  ) {
    _crossRankPeriodicStaging.push_back(
      CrossRankPeriodicEntry<ElementType>{sendTree, recvTree, stamp, depositStack, std::move(data)}
    );
  }

  std::vector<CrossRankPeriodicEntry<ElementType>>& getCrossRankPeriodicStaging() { return _crossRankPeriodicStaging; }

  void clearCrossRankPeriodicStaging() { _crossRankPeriodicStaging.clear(); }

  static constexpr auto LocalStackCount       = StackTopology::TotalNumberOfLocalExchangeStacks;
  static constexpr auto ParallelStacksPerSlot = static_cast<int>(ParallelExchangeStack::NumberOfStacks);

  using LocalStacksPerTree       = std::array<Stack, static_cast<std::size_t>(LocalStackCount)>;
  using ParallelStacksForOneSlot = std::array<Stack, static_cast<std::size_t>(ParallelStacksPerSlot)>;

  void setLocalTrees(const std::set<int>& localTreeIds) {
    std::vector<PerTreeStorage>      newTrees(localTreeIds.size());
    std::vector<std::pair<int, int>> newMap;
    newMap.reserve(localTreeIds.size());

    int idx = 0;
    for (const int treeId : localTreeIds) {
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

    int localIndex = ensureLocalTreeIndex(treeId);
    return _trees[localIndex].local[localStackId];
  }

  /**
   * Stamp list running in lockstep with the payload of a local exchange
   * stack, so that periodic boundary data can be paired with its wrapped
   * partner by identity rather than by traversal order. Only the periodic
   * boundary stacks carry entries. See PeriodicTopology.h.
   */
  std::vector<PeriodicStamp>& getLocalExchangeStamps(int treeId, int localStackId) {
    assertion(localStackId >= 0);
    assertion(localStackId < LocalStackCount);
    return _trees[ensureLocalTreeIndex(treeId)].localStamps[localStackId];
  }

  void pushLocalExchangeStamp(int treeId, int localStackId, const PeriodicStamp& stamp) {
    getLocalExchangeStamps(treeId, localStackId).push_back(stamp);
  }

  /**
   * Remove and return the entry on the given periodic input stack that is the
   * wrapped partner of readerStamp; see peano4::grid::popPeriodicEntryMatching().
   */
  std::optional<ElementType>
  popLocalExchangeStackEntryMatching(int treeId, int localStackId, const PeriodicStamp& readerStamp) {
    return popPeriodicEntryMatching(
      getLocalExchangeStack(treeId, localStackId),
      getLocalExchangeStamps(treeId, localStackId),
      readerStamp
    );
  }

  // TODO: Should be resized when we set the local trees?
  void resizeParallelExchangeStacks(int treeId, int numberOfSlots) {
    assertion(numberOfSlots >= 0);
    const int localIndex = ensureLocalTreeIndex(treeId);
    if (_trees[localIndex].parallel.size() < static_cast<std::size_t>(numberOfSlots)) {
      _trees[localIndex].parallel.resize(static_cast<std::size_t>(numberOfSlots));
    }
  }

  Stack& getParallelExchangeStack(int treeId, int slot, ParallelExchangeStack which) {
    assertion(slot >= 0);

    const int localIndex = ensureLocalTreeIndex(treeId);

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
    // Parallel to `local`: stamps for periodic boundary stacks (others stay empty).
    std::array<std::vector<PeriodicStamp>, static_cast<std::size_t>(LocalStackCount)> localStamps;
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

  /**
   * Index of treeId's storage, registering it on demand if it is not present.
   *
   * setLocalTrees() registers the container's trees once, before the traversal.
   * A worker tree can however be created later in the same sweep, mid-exchange,
   * when peano4::parallel::SpacetreeSet answers a split request from inside
   * receiveDanglingMessages() during MPI progress (see the comment at
   * SpacetreeSet::streamDataFromSplittingTreesToNewTrees). Such a tree is in the
   * spacetree set but not yet in this data container, so we lazily allocate empty
   * storage for it here rather than requiring it to be pre-registered. Registered
   * trees are unaffected, and the next setLocalTrees() re-establishes the
   * canonical ordering, preserving this storage if the tree is now official.
   */
  int ensureLocalTreeIndex(int treeId) {
    const int existing = tryGetLocalTreeIndex(treeId);
    if (existing >= 0) {
      return existing;
    }
    const int newIndex = static_cast<int>(_trees.size());
    _trees.emplace_back();
    _treeIdToLocalIndex.emplace_back(treeId, newIndex);
    std::sort(_treeIdToLocalIndex.begin(), _treeIdToLocalIndex.end());
    return newIndex;
  }

  std::vector<PerTreeStorage> _trees;
  // Sorted by tree id.
  std::vector<std::pair<int, int>> _treeIdToLocalIndex;

  std::vector<CrossRankPeriodicEntry<ElementType>> _crossRankPeriodicStaging;
};
