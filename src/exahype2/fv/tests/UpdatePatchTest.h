// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class UpdatePatchTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::UpdatePatchTest final : public tarch::tests::TestCase {
public:
  UpdatePatchTest();
  UpdatePatchTest(const UpdatePatchTest&)            = delete;
  UpdatePatchTest(UpdatePatchTest&&)                 = delete;
  UpdatePatchTest& operator=(const UpdatePatchTest&) = delete;
  UpdatePatchTest& operator=(UpdatePatchTest&&)      = delete;
  ~UpdatePatchTest() override                        = default;

  void run() override;

private:
  void testUpdateAdvancesThePatchAndTheLabels();
  void testUpdateReportsTheEigenvalueOfAnAdaptiveSolver();
  void testPostprocessedStateBoundsTheTimeStep();
  void testCellOutsideATimeStepOnlyAddsItsStatistics();
};
