// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class DynamicAMRTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::DynamicAMRTest final : public tarch::tests::TestCase {
public:
  DynamicAMRTest();
  DynamicAMRTest(const DynamicAMRTest&)            = delete;
  DynamicAMRTest(DynamicAMRTest&&)                 = delete;
  DynamicAMRTest& operator=(const DynamicAMRTest&) = delete;
  DynamicAMRTest& operator=(DynamicAMRTest&&)      = delete;
  ~DynamicAMRTest() override                       = default;

  void run() override;

private:
  void testCreateFaceClearsTheBuffersAndTakesTheParentTimeStamp();
  void testCreateCellClearsTheStagesDuringGridInitialisation();
  void testCreateCellProlongsEveryStageOfARunningTimeStep();
  void testDestroyCellRestrictsEveryBufferAndFlagsTheParentFaces();
  void testRestrictHangingFaceTraceReducesTheSubfacesAndEvaluatesTheCoarseFlux();
  void testProjectStageAfterTransferMarksTheOwnFaceSides();
  void testProjectStageAfterTransferSkipsTheGridInitialisation();
  void testCellThatOwesDataIntegratesItsVolumeAgain();
};
