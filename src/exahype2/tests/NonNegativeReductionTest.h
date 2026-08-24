// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class NonNegativeReductionTest;
} // namespace exahype2::tests

class exahype2::tests::NonNegativeReductionTest final : public tarch::tests::TestCase {
public:
  NonNegativeReductionTest();

  void run() override;

private:
  void testPhysicalValuesReduceLikeTheStandardOperations();
  void testInvalidObservationsSurviveFurtherReductions();
};
