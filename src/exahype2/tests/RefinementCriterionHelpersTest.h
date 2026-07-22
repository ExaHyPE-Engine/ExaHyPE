// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class RefinementCriterionHelpersTest;
} // namespace exahype2::tests

class exahype2::tests::RefinementCriterionHelpersTest final : public tarch::tests::TestCase {
public:
  RefinementCriterionHelpersTest();

  void run() override;

private:
  void testRefinementCommandFromDetail();
  void testRefineOnDetail();
};
