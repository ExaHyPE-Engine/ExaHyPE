// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox::loadbalancing {
  class Blacklist;
}

class toolbox::loadbalancing::Blacklist final {
public:
  static constexpr int InitialLifetime = 3;

  struct TreeState {
    int  numberOfLocalUnrefinedCells;
    bool removedEmptySubtree;
  };

  template <typename StateOfTree>
  void update(StateOfTree&& stateOfTree);
  void notifySplit(const int treeId, const int numberOfLocalUnrefinedCells);

  bool isBlacklisted(const int treeId) const;

private:
  struct Entry {
    int  _remainingLifetime              = 0;
    int  _baselineNumberOfUnrefinedCells = 0;
    bool _observedLoadChange             = true;
  };

  std::unordered_map<int, Entry> _blacklist;

  static bool isActive(const Entry& entry);
};

/**
 * A source stays unavailable until its fork changes the observed load and @ref InitialLifetime updates have passed.
 * The fork spans two traversals, while globally reduced split state arrives one update later. Removing an empty child
 * restarts both conditions because another split could repeat the degeneration.
 */
template <typename StateOfTree>
void toolbox::loadbalancing::Blacklist::update(StateOfTree&& stateOfTree) {
  auto entryIterator = _blacklist.begin();
  while (entryIterator != _blacklist.end()) {
    auto& [treeId, entry] = *entryIterator;
    const auto state      = stateOfTree(treeId);
    if (state.removedEmptySubtree) {
      entry._remainingLifetime              = InitialLifetime;
      entry._baselineNumberOfUnrefinedCells = state.numberOfLocalUnrefinedCells;
      entry._observedLoadChange             = false;
      ++entryIterator;
      continue;
    }

    if (state.numberOfLocalUnrefinedCells != entry._baselineNumberOfUnrefinedCells) {
      entry._observedLoadChange = true;
    }
    if (entry._remainingLifetime > 0) {
      --entry._remainingLifetime;
    }
    if (isActive(entry)) {
      ++entryIterator;
    } else {
      entryIterator = _blacklist.erase(entryIterator);
    }
  }
}
