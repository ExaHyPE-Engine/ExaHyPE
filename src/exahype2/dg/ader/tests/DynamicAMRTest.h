// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class DynamicAMRTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the traversal-event bodies of the ADER-DG mesh transfer.
 */
class exahype2::dg::ader::tests::DynamicAMRTest final : public tarch::tests::TestCase {
public:
  DynamicAMRTest();
  DynamicAMRTest(const DynamicAMRTest&)            = delete;
  DynamicAMRTest(DynamicAMRTest&&)                 = delete;
  DynamicAMRTest& operator=(const DynamicAMRTest&) = delete;
  DynamicAMRTest& operator=(DynamicAMRTest&&)      = delete;
  ~DynamicAMRTest() override                       = default;

  void run() override;

private:
  void testInterpolateHangingFaceFillsTheCoarseHalf();
  void testStagedFaceFluxHalvesReduceOntoTheCoarseFace();
  void testCreateCellProlongsBothBuffers();
  void testDestroyCellRestrictsBothBuffersAndFlagsTheParentFaces();
  void testReleaseFacesAfterSubcellTransferClearsThePersistentFaces();
  void testRederivePredictionWaitsForAFaceThatAwaitsData();
  void testHangingFacesTakeARederivedTraceAndARiemannSolve();
  void testRederivePredictionWaitsForASettledMeshAndAnAdvancedCell();
};
