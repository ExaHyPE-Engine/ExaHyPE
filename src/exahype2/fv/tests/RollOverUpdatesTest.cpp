// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RollOverUpdatesTest.h"

#include "exahype2/fv/RollOverUpdates.h"
#include "exahype2/fv/tests/ActionSetTestData.h"
#include "peano4/Constants.h"

namespace {
  constexpr int PatchSize        = 3;
  constexpr int HaloSize         = 1;
  constexpr int NumberOfUnknowns = 2;

  constexpr double NewTimeStampOfTheLeftHalf      = 2.0;
  constexpr double NewTimeStampOfTheRightHalf     = 3.0;
  constexpr double UpdatedTimeStampOfTheLeftHalf  = 4.0;
  constexpr double UpdatedTimeStampOfTheRightHalf = 5.0;
  constexpr double TimeStampBeforeTheRollOver     = 1.0;

  constexpr int volumesPerOverlap() {
    int product = 2 * HaloSize;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int OverlapValues = volumesPerOverlap() * NumberOfUnknowns;
  constexpr int PatchValues   = 4;

  peano4::dataview::FaceMarker createMarker() {
    peano4::dataview::FaceMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0;
      marker.cellCentre[axis] = 0.5;
    }
    marker.cellIsLocal = true;
    marker.select(0);
    return marker;
  }
} // namespace

exahype2::fv::tests::RollOverUpdatesTest::RollOverUpdatesTest() :
  tarch::tests::TestCase("exahype2::fv::tests::RollOverUpdatesTest") {}

void exahype2::fv::tests::RollOverUpdatesTest::run() {
  testMethod(testUpdatedHalfIsCopiedAndBothHalvesMoveTheirTimeStamps);
  testMethod(testHalfWithoutATransferRollsOverAfterATopologyChange);
  testMethod(testCellSolutionSwapsOnlyAfterAnUpdate);
}

/**
 * Checks that the roll-over copies the half a sweep has written, leaves the other half of the
 * halo alone and moves the time stamps of both halves on by one time step.
 */
void exahype2::fv::tests::RollOverUpdatesTest::testUpdatedHalfIsCopiedAndBothHalvesMoveTheirTimeStamps() {
  exahype2::fv::tests::Patch<OverlapValues> newHalo;
  exahype2::fv::tests::Patch<OverlapValues> oldHalo;
  for (int value = 0; value < OverlapValues; value++) {
    newHalo.data()[value] = value + 1.0;
  }

  exahype2::fv::tests::FaceLabel faceLabel;
  faceLabel.setUpdated(0, true);
  faceLabel.setNewTimeStamp(0, NewTimeStampOfTheLeftHalf);
  faceLabel.setNewTimeStamp(1, NewTimeStampOfTheRightHalf);
  faceLabel.setUpdatedTimeStamp(0, UpdatedTimeStampOfTheLeftHalf);
  faceLabel.setUpdatedTimeStamp(1, UpdatedTimeStampOfTheRightHalf);
  faceLabel.setOldTimeStamp(0, TimeStampBeforeTheRollOver);
  faceLabel.setOldTimeStamp(1, TimeStampBeforeTheRollOver);

  exahype2::fv::rollOverFaceHalo<NumberOfUnknowns, PatchSize, HaloSize>(
    createMarker(),
    faceLabel,
    newHalo.data(),
    oldHalo.data(),
    false
  );

  // The selected face has its normal along the first axis, whose layer index carries a stride of
  // one in an overlap of two layers, so the left half holds the volumes of even index.
  for (int volume = 0; volume < volumesPerOverlap(); volume++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int    value    = volume * NumberOfUnknowns + unknown;
      const double expected = volume % 2 == 0 ? newHalo.data()[value] : 0.0;
      validateNumericalEqualsWithParams1(oldHalo.data()[value], expected, value);
    }
  }

  validateEquals(faceLabel.getUpdated(0), false);
  validateNumericalEquals(faceLabel.getOldTimeStamp(0), NewTimeStampOfTheLeftHalf);
  validateNumericalEquals(faceLabel.getNewTimeStamp(0), UpdatedTimeStampOfTheLeftHalf);
  validateNumericalEquals(faceLabel.getOldTimeStamp(1), NewTimeStampOfTheRightHalf);
  validateNumericalEquals(faceLabel.getNewTimeStamp(1), UpdatedTimeStampOfTheRightHalf);
}

/**
 * Checks that a half no sweep has updated rolls over after a realised topology change, and only
 * when it awaits no data of its own.
 */
void exahype2::fv::tests::RollOverUpdatesTest::testHalfWithoutATransferRollsOverAfterATopologyChange() {
  exahype2::fv::tests::Patch<OverlapValues> newHalo;
  exahype2::fv::tests::Patch<OverlapValues> oldHalo;
  for (int value = 0; value < OverlapValues; value++) {
    newHalo.data()[value] = value + 1.0;
  }

  exahype2::fv::tests::FaceLabel faceLabel;
  faceLabel.setRequiresDataTransfer(0, true);

  exahype2::fv::rollOverFaceHalo<NumberOfUnknowns, PatchSize, HaloSize>(
    createMarker(),
    faceLabel,
    newHalo.data(),
    oldHalo.data(),
    false
  );
  for (int value = 0; value < OverlapValues; value++) {
    validateNumericalEqualsWithParams1(oldHalo.data()[value], 0.0, value);
  }

  exahype2::fv::rollOverFaceHalo<NumberOfUnknowns, PatchSize, HaloSize>(
    createMarker(),
    faceLabel,
    newHalo.data(),
    oldHalo.data(),
    true
  );
  for (int volume = 0; volume < volumesPerOverlap(); volume++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int    value    = volume * NumberOfUnknowns + unknown;
      const double expected = volume % 2 == 1 ? newHalo.data()[value] : 0.0;
      validateNumericalEqualsWithParams1(oldHalo.data()[value], expected, value);
    }
  }
}

/**
 * Checks that the two patches of a cell change places once and only after an update has written
 * the new patch.
 */
void exahype2::fv::tests::RollOverUpdatesTest::testCellSolutionSwapsOnlyAfterAnUpdate() {
  exahype2::fv::tests::CellLabel          cellLabel;
  exahype2::fv::tests::Patch<PatchValues> oldPatch;
  exahype2::fv::tests::Patch<PatchValues> newPatch;
  for (int value = 0; value < PatchValues; value++) {
    oldPatch.data()[value] = value + 1.0;
    newPatch.data()[value] = value + 100.0;
  }

  exahype2::fv::rollOverCellSolution(cellLabel, oldPatch, newPatch);
  for (int value = 0; value < PatchValues; value++) {
    validateNumericalEqualsWithParams1(oldPatch.data()[value], value + 1.0, value);
  }

  cellLabel.setHasUpdated(true);
  exahype2::fv::rollOverCellSolution(cellLabel, oldPatch, newPatch);
  validateEquals(cellLabel.getHasUpdated(), false);
  for (int value = 0; value < PatchValues; value++) {
    validateNumericalEqualsWithParams1(oldPatch.data()[value], value + 100.0, value);
    validateNumericalEqualsWithParams1(newPatch.data()[value], value + 1.0, value);
  }
}
