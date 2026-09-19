// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class SubcellLayoutTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the node and subcell counts the limiter derives from the order of a solver.
 */
class exahype2::dg::limiting::tests::SubcellLayoutTest final : public tarch::tests::TestCase {
public:
  SubcellLayoutTest();
  SubcellLayoutTest(const SubcellLayoutTest&)            = delete;
  SubcellLayoutTest(SubcellLayoutTest&&)                 = delete;
  SubcellLayoutTest& operator=(const SubcellLayoutTest&) = delete;
  SubcellLayoutTest& operator=(SubcellLayoutTest&&)      = delete;
  ~SubcellLayoutTest() override                          = default;

  void run() override;

private:
  void testTheCountsFollowTheOrderAndTheUnknowns();
};
