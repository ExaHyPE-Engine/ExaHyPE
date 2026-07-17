// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Blacklist.h"

bool toolbox::loadbalancing::Blacklist::isBlacklisted(const int treeId) const {
  const auto entry = _blacklist.find(treeId);
  return entry != _blacklist.end() and isActive(entry->second);
}

bool toolbox::loadbalancing::Blacklist::isActive(const Entry& entry) {
  return entry._remainingLifetime > 0 or not entry._observedLoadChange;
}

void toolbox::loadbalancing::Blacklist::notifySplit(const int treeId, const int numberOfLocalUnrefinedCells) {
  auto& entry                           = _blacklist[treeId];
  entry._remainingLifetime              = InitialLifetime;
  entry._baselineNumberOfUnrefinedCells = numberOfLocalUnrefinedCells;
  entry._observedLoadChange             = false;
}
