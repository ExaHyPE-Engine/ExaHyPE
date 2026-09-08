// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class LimiterTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::LimiterTest final : public tarch::tests::TestCase {
public:
  LimiterTest();

  void run() override;

private:
  void testExtremaMerges();
  void testDiscreteMaximumPrinciple();
  void testObservableExtremaReduction();
  void testProjectedObservableExtremaReduction();
  void testExtremumQuantisation();
  void testHangingVertexFacet();
};
