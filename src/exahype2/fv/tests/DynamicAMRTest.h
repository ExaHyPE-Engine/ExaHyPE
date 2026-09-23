// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class DynamicAMRTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::DynamicAMRTest final : public tarch::tests::TestCase {
public:
  DynamicAMRTest();
  DynamicAMRTest(const DynamicAMRTest&)            = delete;
  DynamicAMRTest(DynamicAMRTest&&)                 = delete;
  DynamicAMRTest& operator=(const DynamicAMRTest&) = delete;
  DynamicAMRTest& operator=(DynamicAMRTest&&)      = delete;
  ~DynamicAMRTest() override                       = default;

  void run() override;

private:
  void testClearFaceAndCellPatch();
  void testInterpolateHangingFaceFillsTheHaloAndTheTimeStamps();
  void testRestrictHangingFaceStagesItsHaloAndRaisesTheCoarseLabel();
  void testRestrictFaceHalvesAveragesTheStagedSubfaces();
  void testClearCoarseningFaceAndCellActOnAParentOnly();
  void testRestrictCoarseningFaceHalvesDiscardsACompletedHalf();
  void testCreateFaceAndDestroyFaceCarryTheTimeStamps();
  void testCreateCellAndDestroyCellReproduceAConstantPatch();
  void testProjectPatchAfterTransferWaitsForAFaceThatAwaitsData();
};
