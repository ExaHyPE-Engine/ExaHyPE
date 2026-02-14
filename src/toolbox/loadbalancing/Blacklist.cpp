// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Blacklist.h"

#include "peano4/parallel/SpacetreeSet.h"

#include <algorithm>
#include <sstream>

tarch::logging::Log toolbox::loadbalancing::Blacklist::_log("toolbox::loadbalancing::Blacklist");

namespace {
  inline decltype(auto) gridStats(int treeId) {
    return peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(treeId);
  }

  inline int localUnrefinedCells(int treeId) {
    const auto& stats = gridStats(treeId);
    return stats.numberOfLocalUnrefinedCells;
  }
} // namespace

toolbox::loadbalancing::Blacklist::Entry::Entry(int treeId) {
  lifetime                       = Blacklist::InitialLifetime;
  baselineNumberOfUnrefinedCells = localUnrefinedCells(treeId);
  numberOfSplits                 = 1;
  numberOfDegeneratedChildren    = 0;
}

std::string toolbox::loadbalancing::Blacklist::toString() const {
  std::ostringstream msg;
  msg << "Blacklist{";

  if (_blacklist.empty()) {
    msg << "Empty";
  } else {
    bool first = true;
    for (const auto& p : _blacklist) {
      if (not first) {
        msg << " ";
      }
      first = false;

      const int    treeId = p.first;
      const Entry& e      = p.second;

      msg << "(#" << treeId << ":";

      if (e.lifetime > 0) {
        msg << "Lifetime=" << e.lifetime << ", #Cells=" << e.baselineNumberOfUnrefinedCells;
      } else {
        msg << "Inactive";
      }

      msg << ", #Splits=" << e.numberOfSplits << ", #DegenerateChildren=" << e.numberOfDegeneratedChildren << ")";
    }
  }

  msg << "}";
  return msg.str();
}

bool toolbox::loadbalancing::Blacklist::isBlacklisted(int treeId) const {
  const auto it = _blacklist.find(treeId);
  return it != _blacklist.end() and it->second.lifetime > 0;
}

void toolbox::loadbalancing::Blacklist::notifySplit(int treeId) {
  auto it = _blacklist.find(treeId);
  if (it == _blacklist.end()) {
    _blacklist.emplace(treeId, Entry(treeId));
    return;
  }

  Entry&     e         = it->second;
  const bool wasActive = (e.lifetime > 0);

  // Always count this split notification and refresh the baseline snapshot.
  e.numberOfSplits++;
  e.baselineNumberOfUnrefinedCells = localUnrefinedCells(treeId);

  // Ensure the entry stays blacklisted for at least InitialLifetime iterations
  // from the most recent split trigger.
  e.lifetime = std::max(e.lifetime, InitialLifetime);

  if (wasActive) {
    logDebug(
      "triggeredSplit()",
      "Split tree "
        << treeId
        << " though it had been on the blacklist. This can happen if a tree splits multiple times in one iteration."
    );
  }
}

void toolbox::loadbalancing::Blacklist::update() {
  for (auto& p : _blacklist) {
    const int treeId = p.first;
    Entry&    e      = p.second;

    const auto& stats = gridStats(treeId);

    // Degeneration handling: extend / enable blacklist and refresh baseline.
    if (stats.getRemovedEmptySubtree()) {
      logDebug("update()", "Tree " << treeId << " has degenerated child. Set/keep on blacklist");
      e.numberOfDegeneratedChildren++;
      e.baselineNumberOfUnrefinedCells = stats.numberOfLocalUnrefinedCells;

      if (e.lifetime > 0) {
        e.lifetime++;
      } else {
        e.lifetime = InitialLifetime;
      }
      continue;
    }

    // Inactive entries stay in the map but do not affect balancing.
    if (e.lifetime <= 0) {
      continue;
    }

    // If the number of cells has not changed since the baseline snapshot,
    //    keep the entry active (prevents repeated splitting attempts).
    const int currentCells = stats.numberOfLocalUnrefinedCells;
    if (currentCells == e.baselineNumberOfUnrefinedCells) {
      logDebug(
        "update()",
        "Tree "
          << treeId
          << " is on blacklist but seems to have failed to fork off cells successfully (local cell count unchanged). Keep on blacklist."
      );
      continue;
    }

    // Progress observed: count down and deactivate when the counter reaches 0.
    if (e.lifetime == 1) {
      logDebug("update()", "Remove tree " << treeId << " from blacklist (keep entry but mark inactive)");
      e.lifetime = 0;
    } else {
      e.lifetime--;
    }
  }
}
