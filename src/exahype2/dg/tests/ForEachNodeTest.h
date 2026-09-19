// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class ForEachNodeTest;
} // namespace exahype2::dg::tests

/**
 * Checks the loop over the nodes of a discontinuous Galerkin patch.
 */
class exahype2::dg::tests::ForEachNodeTest final : public tarch::tests::TestCase {
public:
  ForEachNodeTest();
  ForEachNodeTest(const ForEachNodeTest&)            = delete;
  ForEachNodeTest(ForEachNodeTest&&)                 = delete;
  ForEachNodeTest& operator=(const ForEachNodeTest&) = delete;
  ForEachNodeTest& operator=(ForEachNodeTest&&)      = delete;
  ~ForEachNodeTest() override                        = default;

  void run() override;

private:
  void testVisitsEveryNodeWithItsQuadratureWeight();
  void testReadsFloatValuesInPlace();
};
