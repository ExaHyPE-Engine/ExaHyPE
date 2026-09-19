// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchOntoFacesTest.h"

#include "exahype2/fv/ProjectPatchOntoFaces.h"
#include "exahype2/fv/tests/ActionSetTestData.h"
#include "peano4/Constants.h"

namespace {
  constexpr int    PatchSize          = 2;
  constexpr int    Overlap            = 1;
  constexpr int    NumberOfUnknowns   = 1;
  constexpr double TimeStampOfTheCell = 2.5;

  constexpr int volumesPerPatch() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int volumesPerOverlap() {
    int product = 2 * Overlap;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int PatchValues   = volumesPerPatch() * NumberOfUnknowns;
  constexpr int OverlapValues = volumesPerOverlap() * NumberOfUnknowns;

  /**
   * Every volume carries its position along the first axis plus one, so a projection along that
   * axis writes one value.
   */
  struct TestCell final {
    exahype2::fv::tests::Patch<PatchValues>                                     patch{};
    std::array<exahype2::fv::tests::FaceLabel, peano4::FacesPerCell>            faceLabels{};
    std::array<exahype2::fv::tests::Patch<OverlapValues>, peano4::FacesPerCell> faceData{};
  };

  TestCell createCell() {
    TestCell cell;
    for (int volume = 0; volume < volumesPerPatch(); volume++) {
      cell.patch.data()[volume] = volume % PatchSize + 1.0;
    }
    for (auto& faceLabel : cell.faceLabels) {
      faceLabel.setRequiresDataTransfer(0, true);
      faceLabel.setRequiresDataTransfer(1, true);
    }
    return cell;
  }

  void projectOneCell(TestCell& cell) {
    exahype2::fv::projectPatchOntoFaces<PatchSize, Overlap, NumberOfUnknowns, double>(
      exahype2::fv::tests::enumerateFaces(cell.faceLabels),
      cell.patch.data(),
      exahype2::fv::tests::enumerateFaces(cell.faceData),
      TimeStampOfTheCell
    );
  }
} // namespace

exahype2::fv::tests::ProjectPatchOntoFacesTest::ProjectPatchOntoFacesTest() :
  tarch::tests::TestCase("exahype2::fv::tests::ProjectPatchOntoFacesTest") {}

void exahype2::fv::tests::ProjectPatchOntoFacesTest::run() {
  testMethod(testProjectionFillsTheHalfOfEveryFaceTheCellOwns);
  testMethod(testProjectionStampsTheFaceLabels);
}

/**
 * Checks that the projection writes the boundary volumes of the patch into the half of each face
 * overlap the cell owns and leaves the half of the neighbour untouched.
 */
void exahype2::fv::tests::ProjectPatchOntoFacesTest::testProjectionFillsTheHalfOfEveryFaceTheCellOwns() {
  TestCell cell = createCell();
  projectOneCell(cell);

  // The normal of the first face runs along the first axis, whose layer index carries a stride of
  // one in an overlap of two layers, so the cell owns the volumes of odd index there and the
  // volumes of even index in the opposite face.
  for (int volume = 0; volume < volumesPerOverlap(); volume++) {
    const double lowerFace = volume % 2 == 0 ? 0.0 : 1.0;
    const double upperFace = volume % 2 == 0 ? PatchSize : 0.0;
    validateNumericalEqualsWithParams1(cell.faceData[0].data()[volume], lowerFace, volume);
    validateNumericalEqualsWithParams1(cell.faceData[DIMENSIONS].data()[volume], upperFace, volume);
  }

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    int writtenValues = 0;
    for (int value = 0; value < OverlapValues; value++) {
      if (cell.faceData[faceNumber].data()[value] != 0.0) {
        writtenValues++;
      }
    }
    validateEqualsWithParams1(writtenValues, OverlapValues / 2, faceNumber);
  }
}

/**
 * Checks that every face half the projection writes carries the time stamp of the cell, its
 * updated flag and no request for a mesh transfer.
 */
void exahype2::fv::tests::ProjectPatchOntoFacesTest::testProjectionStampsTheFaceLabels() {
  TestCell cell = createCell();
  projectOneCell(cell);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateEqualsWithParams1(cell.faceLabels[axis].getUpdated(1), true, axis);
    validateEqualsWithParams1(cell.faceLabels[axis].getUpdated(0), false, axis);
    validateEqualsWithParams1(cell.faceLabels[axis + DIMENSIONS].getUpdated(0), true, axis);
    validateEqualsWithParams1(cell.faceLabels[axis + DIMENSIONS].getUpdated(1), false, axis);

    validateNumericalEqualsWithParams1(cell.faceLabels[axis].getUpdatedTimeStamp(1), TimeStampOfTheCell, axis);
    validateNumericalEqualsWithParams1(
      cell.faceLabels[axis + DIMENSIONS].getUpdatedTimeStamp(0),
      TimeStampOfTheCell,
      axis
    );

    validateEqualsWithParams1(cell.faceLabels[axis].getRequiresDataTransfer(1), false, axis);
    validateEqualsWithParams1(cell.faceLabels[axis].getRequiresDataTransfer(0), true, axis);
    validateEqualsWithParams1(cell.faceLabels[axis + DIMENSIONS].getRequiresDataTransfer(0), false, axis);
    validateEqualsWithParams1(cell.faceLabels[axis + DIMENSIONS].getRequiresDataTransfer(1), true, axis);
  }
}
