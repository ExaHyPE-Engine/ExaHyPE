// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace loadbalancing {
    namespace tests {
      class StatisticsTest;
    } // namespace tests
  } // namespace loadbalancing
} // namespace toolbox

class toolbox::loadbalancing::tests::StatisticsTest final: public tarch::tests::TestCase {
private:
  void testPackedReductionMatchesSimpleMPI();
  void testStateUpdatesWithoutSplitCounter();

public:
  StatisticsTest();
  ~StatisticsTest() override final = default;

  void run() override final;
};
