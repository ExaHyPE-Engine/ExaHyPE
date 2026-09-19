// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceStatusTest.h"

#include "exahype2/dg/limiting/FaceStatus.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"

namespace {
  using FaceLabel     = exahype2::dg::limiting::tests::TestFaceLabel;
  using FaceLabels    = exahype2::dg::limiting::tests::TestEnumerator<FaceLabel, peano4::FacesPerCell>;
  using LimiterStatus = exahype2::dg::LimiterStatus;
} // namespace

exahype2::dg::limiting::tests::FaceStatusTest::FaceStatusTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::FaceStatusTest") {}

void exahype2::dg::limiting::tests::FaceStatusTest::run() {
  testMethod(testCellWritesItsStatusOntoTheOwnedFaceSide);
  testMethod(testSnapshotKeepsTheLiveStatusAndClearsTheRefinedSide);
  testMethod(testHangingFaceTakesBothSidesFromTheCoarseSnapshot);
  testMethod(testHangingFaceRaisesItsStatusOntoTheCoarseFace);
}

/**
 * Checks that a cell's status reaches the right side of its lower faces and the left side of its upper ones.
 */
void exahype2::dg::limiting::tests::FaceStatusTest::testCellWritesItsStatusOntoTheOwnedFaceSide() {
  TestCellLabel cellLabel;
  cellLabel.setLimiterStatus(LimiterStatus::Troubled);
  FaceLabel        faces[peano4::FacesPerCell];
  const FaceLabels faceLabels{faces};

  exahype2::dg::limiting::projectCellStatusOntoFaces(cellLabel, faceLabels);

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int ownSide = faceNumber < DIMENSIONS ? 1 : 0;
    validateEquals(
      static_cast<int>(faces[faceNumber].getLimiterStatus(ownSide)),
      static_cast<int>(LimiterStatus::Troubled)
    );
    validateEquals(
      static_cast<int>(faces[faceNumber].getLimiterStatus(1 - ownSide)),
      static_cast<int>(LimiterStatus::Regular)
    );
  }
}

/**
 * Checks that the snapshot holds the status of the closed ring while the refined side resets.
 */
void exahype2::dg::limiting::tests::FaceStatusTest::testSnapshotKeepsTheLiveStatusAndClearsTheRefinedSide() {
  FaceLabel faceLabel;
  faceLabel.setLimiterStatus(0, LimiterStatus::Limited);
  faceLabel.setLimiterStatus(1, LimiterStatus::Troubled);
  faceLabel.aboveHanging(1) = 1;

  exahype2::dg::limiting::snapshotFaceStatus(faceLabel);

  validateEquals(static_cast<int>(faceLabel.getLimiterStatusSnapshot(0)), static_cast<int>(LimiterStatus::Limited));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatusSnapshot(1)), static_cast<int>(LimiterStatus::Troubled));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatus(0)), static_cast<int>(LimiterStatus::Limited));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatus(1)), static_cast<int>(LimiterStatus::Regular));
}

/**
 * Checks that a hanging face starts from the coarse face's snapshot on both sides.
 */
void exahype2::dg::limiting::tests::FaceStatusTest::testHangingFaceTakesBothSidesFromTheCoarseSnapshot() {
  constexpr int SelectedFaceNumber = 0;
  FaceLabel     coarseFaces[peano4::FacesPerCell];
  coarseFaces[SelectedFaceNumber].setLimiterStatusSnapshot(0, LimiterStatus::Halo);
  coarseFaces[SelectedFaceNumber].setLimiterStatusSnapshot(1, LimiterStatus::Limited);
  const FaceLabels coarseFaceLabels{coarseFaces};
  FaceLabel        faceLabel;
  faceLabel.setLimiterStatus(0, LimiterStatus::Troubled);

  exahype2::dg::limiting::copyCoarseFaceStatusOntoHangingFace(SelectedFaceNumber, faceLabel, coarseFaceLabels);

  validateEquals(static_cast<int>(faceLabel.getLimiterStatus(0)), static_cast<int>(LimiterStatus::Halo));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatus(1)), static_cast<int>(LimiterStatus::Limited));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatusSnapshot(0)), static_cast<int>(LimiterStatus::Halo));
  validateEquals(static_cast<int>(faceLabel.getLimiterStatusSnapshot(1)), static_cast<int>(LimiterStatus::Limited));
}

/**
 * Checks that the fine cell's status reaches the refined side of the coarse face as a maximum.
 */
void exahype2::dg::limiting::tests::FaceStatusTest::testHangingFaceRaisesItsStatusOntoTheCoarseFace() {
  constexpr int SelectedFaceNumber = DIMENSIONS;
  constexpr int RefinedSide        = 0;
  FaceLabel     coarseFaces[peano4::FacesPerCell];
  coarseFaces[SelectedFaceNumber].setLimiterStatus(RefinedSide, LimiterStatus::Halo);
  const FaceLabels coarseFaceLabels{coarseFaces};
  FaceLabel        faceLabel;
  faceLabel.setLimiterStatus(RefinedSide, LimiterStatus::Limited);

  exahype2::dg::limiting::raiseHangingFaceStatusOntoCoarseFace(SelectedFaceNumber, faceLabel, coarseFaceLabels);

  validateEquals(
    static_cast<int>(coarseFaces[SelectedFaceNumber].getLimiterStatus(RefinedSide)),
    static_cast<int>(LimiterStatus::Limited)
  );
}
