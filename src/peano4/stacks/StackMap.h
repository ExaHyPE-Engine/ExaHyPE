// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/parallel/Node.h"

#include <map>

namespace peano4::stacks {
  template <class T>
  class StackMap;

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
  typedef std::pair<int, int> StackKey; // (tree number, stack number)
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackMap {
private:
  static tarch::logging::Log _log;

  /**
   * Data of one tree.
   */
  struct TreeData {
    // std::map<int, std::unique_ptr<T>> stackMap;
    std::unordered_map<int, std::unique_ptr<T>> stackMap;
  };

  std::vector<TreeData> _data;

  void createStack(int localTreeNumber, int stackNumber) {
    assertion(localTreeNumber >= 0);
    assertion(localTreeNumber < static_cast<int>(_data.size()));
    assertion(stackNumber >= 0);
    assertion(_data[localTreeNumber].stackMap.count(stackNumber) <= 1);
    auto& stacks = _data[localTreeNumber].stackMap;
    if (stacks.find(stackNumber) == stacks.end()) {
      logDebug("createStack()", "Created stack " << stackNumber << " in local tree " << localTreeNumber);
      stacks.try_emplace(stackNumber, std::make_unique<T>());
    }
  }

public:
  StackMap():
    _data(parallel::Node::MaxSpacetreesPerRank) {}

  ~StackMap() = default;

  // Returns true if no stack exists for the given tree/stack id or if that stack is empty.
  bool empty(int localTreeNumber, int stackNumber) const {
    assertion2(localTreeNumber >= 0, localTreeNumber, stackNumber);
    assertion3(localTreeNumber < static_cast<int>(_data.size()), _data.size(), localTreeNumber, stackNumber);
    auto it = _data[localTreeNumber].stackMap.find(stackNumber);
    return (it == _data[localTreeNumber].stackMap.end() or it->second->empty());
  }

  // Clears all stacks for all spacetrees.
  void clear() {
    for (auto& tree : _data) {
      for (auto& entry : tree.stackMap) {
        entry.second->clear();
        // entry.second.reset(new T());
      }
    }
  }

  // Clears all stacks for the given spacetree.
  void clear(int localTreeNumber) {
    assertion1(localTreeNumber >= 0, localTreeNumber);
    if (localTreeNumber < static_cast<int>(_data.size())) {
      for (auto& entry : _data[localTreeNumber].stackMap) {
        entry.second->clear();
        // entry.second.reset(new T());
      }
    }
  }

  // Lazy accessor: returns the stack for push; creates it if missing.
  T* getForPush(int localTreeNumber, int stackNumber) { return getForPush(StackKey(localTreeNumber, stackNumber)); }
  T* getForPush(const StackKey& key) {
    const int localTreeNumber = key.first;
    const int stackNumber     = key.second;
    assertion2(localTreeNumber >= 0, localTreeNumber, stackNumber);
    assertion3(localTreeNumber < static_cast<int>(_data.size()), _data.size(), localTreeNumber, stackNumber);
    createStack(localTreeNumber, stackNumber);
    return _data[localTreeNumber].stackMap[stackNumber].get();
  }

  // Returns the stack for pop; it must already exist.
  T* getForPop(int localTreeNumber, int stackNumber) { return getForPop(StackKey(localTreeNumber, stackNumber)); }
  T* getForPop(const StackKey& key) {
    const int localTreeNumber = key.first;
    const int stackNumber     = key.second;
    assertion2(localTreeNumber >= 0, localTreeNumber, stackNumber);
    assertion3(localTreeNumber < static_cast<int>(_data.size()), _data.size(), localTreeNumber, stackNumber);
    auto it = _data[localTreeNumber].stackMap.find(stackNumber);
    assertion2(it != _data[localTreeNumber].stackMap.end(), localTreeNumber, stackNumber);
    return it->second.get();
  }

  std::string toString() const {
    std::ostringstream oss;
    oss << "StackMap(" << _data.size() << " trees)";
    return oss.str();
  }

  // Returns keys for local tree number and their stack numbers
  std::set<StackKey> getKeys() const {
    std::set<StackKey> keys;
    for (int i = 0; i < static_cast<int>(_data.size()); i++) {
      for (const auto& entry : _data[i].stackMap) {
        keys.insert(StackKey(i, entry.first));
      }
    }
    return keys;
  }

  // Performs garbage collection on the stacks of a given spacetree.
  // If a stack is empty and its id is not reserved for storage, it is replaced.
  void garbageCollection(int localTreeNumber) {
    if (localTreeNumber < static_cast<int>(_data.size())) {
      for (auto& entry : _data[localTreeNumber].stackMap) {
        if (entry.second->empty() and not parallel::Node::isStorageStackNumber(entry.first)) {
          entry.second->clear();
          // entry.second.reset(new T());
        }
      }
    }
  }

  // Returns true if a stack exists for the given tree/stack id.
  bool holdsStack(int localTreeNumber, int stackNumber) const {
    return holdsStack(StackKey(localTreeNumber, stackNumber));
  }
  bool holdsStack(const StackKey& key) const {
    const int localTreeNumber = key.first;
    const int stackNumber     = key.second;
    assertion2(localTreeNumber >= 0, localTreeNumber, stackNumber);
    assertion3(localTreeNumber < static_cast<int>(_data.size()), _data.size(), localTreeNumber, stackNumber);
    return _data[localTreeNumber].stackMap.count(stackNumber) == 1;
  }
};

template <class T>
tarch::logging::Log peano4::stacks::StackMap<T>::_log("peano4::stacks::StackMap<T>");
