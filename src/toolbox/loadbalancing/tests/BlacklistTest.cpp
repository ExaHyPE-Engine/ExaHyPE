// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BlacklistTest.h"

#include "toolbox/loadbalancing/Blacklist.h"

namespace {
  toolbox::loadbalancing::Blacklist::TreeState
  treeState(const int numberOfLocalUnrefinedCells, const bool removedEmptySubtree = false) {
    return toolbox::loadbalancing::Blacklist::TreeState{numberOfLocalUnrefinedCells, removedEmptySubtree};
  }
} // namespace

toolbox::loadbalancing::tests::BlacklistTest::BlacklistTest() :
  tarch::tests::TestCase("toolbox::loadbalancing::tests::BlacklistTest") {}

void toolbox::loadbalancing::tests::BlacklistTest::run() {
  testMethod(testEmptyBlacklist);
  testMethod(testNotifiedTreeIsBlacklisted);
  testMethod(testElapsedAgeRequiresObservedLoadChange);
  testMethod(testElapsedAgeAndObservedLoadChangeReleaseTree);
  testMethod(testDegenerationRestartsCoolingOff);
  testMethod(testInactiveTreeSkipsCallback);
}

void toolbox::loadbalancing::tests::BlacklistTest::testEmptyBlacklist() {
  Blacklist blacklist;
  validate(not blacklist.isBlacklisted(0));
}

void toolbox::loadbalancing::tests::BlacklistTest::testNotifiedTreeIsBlacklisted() {
  constexpr int TreeId = 7;
  Blacklist     blacklist;
  blacklist.notifySplit(TreeId, 100);
  validate(blacklist.isBlacklisted(TreeId));
  blacklist.notifySplit(TreeId, 75);
  validate(blacklist.isBlacklisted(TreeId));
}

void toolbox::loadbalancing::tests::BlacklistTest::testElapsedAgeRequiresObservedLoadChange() {
  constexpr int TreeId = 3;
  Blacklist     blacklist;
  blacklist.notifySplit(TreeId, 50);
  for (int updateIndex = 0; updateIndex < Blacklist::InitialLifetime; ++updateIndex) {
    blacklist.update([](const int) { return treeState(50); });
  }
  validate(blacklist.isBlacklisted(TreeId));
  blacklist.update([](const int) { return treeState(25); });
  validate(not blacklist.isBlacklisted(TreeId));
}

void toolbox::loadbalancing::tests::BlacklistTest::testElapsedAgeAndObservedLoadChangeReleaseTree() {
  constexpr int TreeId = 4;
  Blacklist     blacklist;
  blacklist.notifySplit(TreeId, 50);
  for (int updateIndex = 0; updateIndex < Blacklist::InitialLifetime; ++updateIndex) {
    validate(blacklist.isBlacklisted(TreeId));
    blacklist.update([](const int) { return treeState(25); });
  }
  validate(not blacklist.isBlacklisted(TreeId));
}

void toolbox::loadbalancing::tests::BlacklistTest::testDegenerationRestartsCoolingOff() {
  constexpr int TreeId = 5;
  Blacklist     blacklist;
  blacklist.notifySplit(TreeId, 50);
  for (int updateIndex = 1; updateIndex < Blacklist::InitialLifetime; ++updateIndex) {
    blacklist.update([](const int) { return treeState(25); });
  }
  blacklist.update([](const int) { return treeState(25, true); });
  for (int updateIndex = 0; updateIndex < Blacklist::InitialLifetime; ++updateIndex) {
    blacklist.update([](const int) { return treeState(25); });
  }
  validate(blacklist.isBlacklisted(TreeId));
  blacklist.update([](const int) { return treeState(20); });
  validate(not blacklist.isBlacklisted(TreeId));
}

void toolbox::loadbalancing::tests::BlacklistTest::testInactiveTreeSkipsCallback() {
  constexpr int TreeId = 6;
  Blacklist     blacklist;
  blacklist.notifySplit(TreeId, 50);
  int numberOfCallbacks = 0;
  for (int updateIndex = 0; updateIndex < Blacklist::InitialLifetime; ++updateIndex) {
    blacklist.update([&numberOfCallbacks](const int) {
      ++numberOfCallbacks;
      return treeState(25);
    });
  }
  validate(not blacklist.isBlacklisted(TreeId));
  validateEquals(numberOfCallbacks, Blacklist::InitialLifetime);
  blacklist.update([&numberOfCallbacks](const int) {
    ++numberOfCallbacks;
    return treeState(25);
  });
  validateEquals(numberOfCallbacks, Blacklist::InitialLifetime);
}
