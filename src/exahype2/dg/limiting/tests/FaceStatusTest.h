// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class FaceStatusTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks how a cell's limiter status reaches its faces and how a hanging face carries it.
 */
class exahype2::dg::limiting::tests::FaceStatusTest final : public tarch::tests::TestCase {
public:
  FaceStatusTest();
  FaceStatusTest(const FaceStatusTest&)            = delete;
  FaceStatusTest(FaceStatusTest&&)                 = delete;
  FaceStatusTest& operator=(const FaceStatusTest&) = delete;
  FaceStatusTest& operator=(FaceStatusTest&&)      = delete;
  ~FaceStatusTest() override                       = default;

  void run() override;

private:
  void testCellWritesItsStatusOntoTheOwnedFaceSide();
  void testSnapshotKeepsTheLiveStatusAndClearsTheRefinedSide();
  void testHangingFaceTakesBothSidesFromTheCoarseSnapshot();
  void testHangingFaceRaisesItsStatusOntoTheCoarseFace();
};
