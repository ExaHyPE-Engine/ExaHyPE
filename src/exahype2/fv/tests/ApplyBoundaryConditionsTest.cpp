// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ApplyBoundaryConditionsTest.h"

#include "exahype2/fv/ApplyBoundaryConditions.h"
#include "exahype2/fv/tests/ActionSetTestData.h"
#include "peano4/Constants.h"

namespace {
  constexpr int    PatchSize            = 4;
  constexpr int    HaloSize             = 1;
  constexpr int    Unknowns             = 2;
  constexpr double ValueInsideThePatch  = 3.0;
  constexpr double TimeStampOfTheFace   = 1.5;
  constexpr double UpdatedTimeStamp     = 7.0;
  constexpr double NewTimeStamp         = 5.0;
  constexpr double ValueOutsideThePatch = ValueInsideThePatch + TimeStampOfTheFace;

  constexpr int volumesPerOverlap() {
    int product = 2 * HaloSize;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int OverlapValues = volumesPerOverlap() * Unknowns;
  constexpr int TreatedFace   = 0;

  /**
   * Adds the time stamp of the face to the inner value and writes the sum into the outer volume,
   * so one value proves both the copy and the time stamp the caller passes.
   */
  struct TestSolver final {
    static constexpr int NumberOfFiniteVolumesPerAxisPerPatch = PatchSize;
    static constexpr int NumberOfUnknowns                     = Unknowns;
    static constexpr int NumberOfAuxiliaryVariables           = 0;

    std::bitset<DIMENSIONS> PeriodicBoundaries{};

    static void boundaryConditions(
      const double* const                                           insideState,
      double* const                                                 outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        outsideState[unknown] = insideState[unknown] + timeStamp;
      }
    }
  };

  /**
   * Every face but the second one along the first axis carries the domain boundary, and every
   * overlap starts at the value of the adjacent patch.
   */
  struct TestFaces final {
    std::array<exahype2::fv::tests::FaceLabel, peano4::FacesPerCell>            labels{};
    std::array<exahype2::fv::tests::Patch<OverlapValues>, peano4::FacesPerCell> data{};
  };

  TestFaces createFaces() {
    TestFaces faces;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int insideEntry = faceNumber < DIMENSIONS ? 1 : 0;
      faces.labels[faceNumber].setBoundary(true);
      faces.labels[faceNumber].setOldTimeStamp(insideEntry, TimeStampOfTheFace);
      faces.labels[faceNumber].setNewTimeStamp(insideEntry, NewTimeStamp);
      faces.labels[faceNumber].setUpdatedTimeStamp(insideEntry, UpdatedTimeStamp);
      faces.data[faceNumber].fill(ValueInsideThePatch);
    }
    faces.labels[DIMENSIONS].setBoundary(false);
    return faces;
  }

  peano4::dataview::CellMarker createMarker() {
    peano4::dataview::CellMarker marker{};
    marker.level = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0;
      marker.centre[axis] = 0.5;
    }
    return marker;
  }

  void applyBoundaryConditionsOnOneCell(TestSolver& solver, TestFaces& faces) {
    exahype2::fv::applyBoundaryConditions<HaloSize, double>(
      createMarker(),
      solver,
      exahype2::fv::tests::enumerateFaces(faces.labels),
      exahype2::fv::tests::enumerateFaces(faces.data)
    );
  }
} // namespace

exahype2::fv::tests::ApplyBoundaryConditionsTest::ApplyBoundaryConditionsTest() :
  tarch::tests::TestCase("exahype2::fv::tests::ApplyBoundaryConditionsTest") {}

void exahype2::fv::tests::ApplyBoundaryConditionsTest::run() {
  testMethod(testDomainFaceTakesTheBoundaryValuesAndTheTimeStamps);
  testMethod(testPeriodicAxisAndInteriorFaceKeepTheirHalo);
}

/**
 * Checks that the outer volumes of the first face take the value of the boundary condition and
 * that the three time stamps of its outer half take the values of its inner half.
 */
void exahype2::fv::tests::ApplyBoundaryConditionsTest::testDomainFaceTakesTheBoundaryValuesAndTheTimeStamps() {
  TestSolver solver;
  TestFaces  faces = createFaces();
  applyBoundaryConditionsOnOneCell(solver, faces);

  // The normal of the first face runs along the first axis, whose layer index carries a stride
  // of one in an overlap of two layers, so the outer volumes are the ones of even index.
  for (int volume = 0; volume < volumesPerOverlap(); volume++) {
    const double expected = volume % 2 == 0 ? ValueOutsideThePatch : ValueInsideThePatch;
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        faces.data[TreatedFace].data()[volume * Unknowns + unknown],
        expected,
        volume,
        unknown
      );
    }
  }

  validateNumericalEquals(faces.labels[TreatedFace].getOldTimeStamp(0), TimeStampOfTheFace);
  validateNumericalEquals(faces.labels[TreatedFace].getNewTimeStamp(0), NewTimeStamp);
  validateNumericalEquals(faces.labels[TreatedFace].getUpdatedTimeStamp(0), UpdatedTimeStamp);
}

/**
 * Checks that a face of a periodic axis and a face away from the domain boundary keep their
 * halo and the time stamps of their outer half.
 */
void exahype2::fv::tests::ApplyBoundaryConditionsTest::testPeriodicAxisAndInteriorFaceKeepTheirHalo() {
  TestSolver solver;
  solver.PeriodicBoundaries.set(DIMENSIONS - 1);
  TestFaces faces = createFaces();
  applyBoundaryConditionsOnOneCell(solver, faces);

  for (const int faceNumber : {DIMENSIONS - 1, DIMENSIONS, 2 * DIMENSIONS - 1}) {
    for (int value = 0; value < OverlapValues; value++) {
      validateNumericalEqualsWithParams2(faces.data[faceNumber].data()[value], ValueInsideThePatch, faceNumber, value);
    }
    const int outsideEntry = faceNumber < DIMENSIONS ? 0 : 1;
    validateNumericalEqualsWithParams1(faces.labels[faceNumber].getOldTimeStamp(outsideEntry), 0.0, faceNumber);
    validateNumericalEqualsWithParams1(faces.labels[faceNumber].getNewTimeStamp(outsideEntry), 0.0, faceNumber);
    validateNumericalEqualsWithParams1(faces.labels[faceNumber].getUpdatedTimeStamp(outsideEntry), 0.0, faceNumber);
  }
}
