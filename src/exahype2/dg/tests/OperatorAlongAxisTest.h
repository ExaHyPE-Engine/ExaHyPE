// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class OperatorAlongAxisTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::OperatorAlongAxisTest final : public tarch::tests::TestCase {
public:
  OperatorAlongAxisTest();
  OperatorAlongAxisTest(const OperatorAlongAxisTest&)            = delete;
  OperatorAlongAxisTest(OperatorAlongAxisTest&&)                 = delete;
  OperatorAlongAxisTest& operator=(const OperatorAlongAxisTest&) = delete;
  OperatorAlongAxisTest& operator=(OperatorAlongAxisTest&&)      = delete;
  ~OperatorAlongAxisTest() override                              = default;

  void run() override;

private:
  void testProductMatchesTheDefinition();
  void testResultIsAccumulatedOrOverwritten();
  void testScalingMultipliesEveryProduct();
  void testStridesAddressTheIntendedSlices();
  void testANarrowerResultIsRoundedOnce();
};
