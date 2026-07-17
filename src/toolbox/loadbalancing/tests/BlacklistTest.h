// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::loadbalancing::tests {
  class BlacklistTest;
} // namespace toolbox::loadbalancing::tests

class toolbox::loadbalancing::tests::BlacklistTest final : public tarch::tests::TestCase {
public:
  BlacklistTest();
  void run() override;

private:
  void testEmptyBlacklist();
  void testNotifiedTreeIsBlacklisted();
  void testElapsedAgeRequiresObservedLoadChange();
  void testElapsedAgeAndObservedLoadChangeReleaseTree();
  void testDegenerationRestartsCoolingOff();
  void testInactiveTreeSkipsCallback();
};
