// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BlacklistTest.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "toolbox/loadbalancing/Blacklist.h"

namespace {
  int extractIntField(const std::string& s, int treeId, const std::string& field) {
    const std::string entryStart = "(#" + std::to_string(treeId) + ":";
    std::size_t       p          = s.find(entryStart);
    if (p == std::string::npos) {
      return -1;
    }

    p = s.find(field, p);
    if (p == std::string::npos) {
      return -1;
    }

    p += field.size();
    int  value = 0;
    bool found = false;
    while (p < s.size() and std::isdigit(static_cast<unsigned char>(s[p]))) {
      found = true;
      value = value * 10 + (s[p] - '0');
      ++p;
    }
    return found ? value : -1;
  }

  int pickNonDegeneratedLocalTreeId() {
    const auto& set   = peano4::parallel::SpacetreeSet::getInstance();
    const auto& trees = set.getLocalSpacetrees();
    for (int t : trees) {
      if (not set.getGridStatistics(t).getRemovedEmptySubtree()) {
        return t;
      }
    }
    return trees.empty() ? -1 : *trees.begin();
  }
} // namespace

toolbox::loadbalancing::tests::BlacklistTest::BlacklistTest():
  tarch::tests::TestCase("toolbox::loadbalancing::tests::BlacklistTest") {}

void toolbox::loadbalancing::tests::BlacklistTest::run() {
  testMethod(testEmptyBlacklist);
  testMethod(testTriggeredSplitAddsEntryAndCountsSplits);
  testMethod(testUpdateKeepsLifetimeIfCellCountUnchanged);
}

void toolbox::loadbalancing::tests::BlacklistTest::testEmptyBlacklist() {
  Blacklist b;

  validate(not b.isBlacklisted(0));

  const std::string s = b.toString();
  validate(s.find("Empty") != std::string::npos);
  validate(s.find("(#") == std::string::npos);
}

void toolbox::loadbalancing::tests::BlacklistTest::testTriggeredSplitAddsEntryAndCountsSplits() {
  const int treeId = pickNonDegeneratedLocalTreeId();
  if (treeId < 0) {
    return;
  }

  Blacklist b;
  b.notifySplit(treeId);

  validate(b.isBlacklisted(treeId));
  {
    const std::string s = b.toString();
    validate(s.find("(#" + std::to_string(treeId) + ":Lifetime=") != std::string::npos);
    validate(extractIntField(s, treeId, "Lifetime=") > 0);
    validateEquals(extractIntField(s, treeId, "#Splits="), 1);
    validateEquals(extractIntField(s, treeId, "#DegenerateChildren="), 0);
  }

  // Second call within the same iteration: should bump #Splits
  b.notifySplit(treeId);
  {
    const std::string s = b.toString();
    validateEquals(extractIntField(s, treeId, "#Splits="), 2);
    validate(extractIntField(s, treeId, "Lifetime=") > 0);
  }
}

void toolbox::loadbalancing::tests::BlacklistTest::testUpdateKeepsLifetimeIfCellCountUnchanged() {
  const int treeId = pickNonDegeneratedLocalTreeId();
  if (treeId < 0) {
    return;
  }

  const auto& set   = peano4::parallel::SpacetreeSet::getInstance();
  const int   cells = set.getGridStatistics(treeId).numberOfLocalUnrefinedCells;

  Blacklist b;
  b.notifySplit(treeId);

  const int lifetimeBeforeUpdate = extractIntField(b.toString(), treeId, "Lifetime=");
  validate(lifetimeBeforeUpdate > 0);

  b.update();
  validate(b.isBlacklisted(treeId));

  // If nothing changed (usual case in the unit test environment), the blacklist
  // entry should remain active and its lifetime should not count down.
  const int         cellsAfter          = set.getGridStatistics(treeId).numberOfLocalUnrefinedCells;
  const std::string s                   = b.toString();
  const int         lifetimeAfterUpdate = extractIntField(s, treeId, "Lifetime=");
  validate(lifetimeAfterUpdate > 0);
  if (cellsAfter == cells) {
    validateEquals(lifetimeAfterUpdate, lifetimeBeforeUpdate);
  } else {
    // Progress observed: lifetime should count down.
    validateEquals(lifetimeAfterUpdate, lifetimeBeforeUpdate - 1);
  }
  validate(s.find("Inactive") == std::string::npos);
}
