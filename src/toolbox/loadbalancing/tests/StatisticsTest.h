// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::loadbalancing::tests {
  class StatisticsTest;
} // namespace toolbox::loadbalancing::tests

class toolbox::loadbalancing::tests::StatisticsTest final : public tarch::tests::TestCase {
public:
  StatisticsTest();
  void run() override;

private:
  void testLocalSnapshotMatchesSpacetreeSet();
  void testHasAdaptiveMesh();
  void testControlledPackedReduction();
  void testWidePackedReduction();
  void testDelayedCollectiveParticipant();
};
