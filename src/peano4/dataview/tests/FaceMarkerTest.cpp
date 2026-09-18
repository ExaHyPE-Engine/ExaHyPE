// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceMarkerTest.h"

#include "peano4/dataview/FaceMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/tests/TestMacros.h"

namespace {
  peano4::dataview::FaceMarker createMarker() {
    peano4::dataview::FaceMarker marker{};
    for (int d = 0; d < DIMENSIONS; d++) {
      marker.h[d]          = 1.0 / 3.0;
      marker.cellCentre[d] = 0.5;
    }
    marker.cellIsLocal = true;
    return marker;
  }
} // namespace

peano4::dataview::tests::FaceMarkerTest::FaceMarkerTest() :
  TestCase("peano4::dataview::tests::FaceMarkerTest") {}

void peano4::dataview::tests::FaceMarkerTest::run() {
  testMethod(testSelection);
  testMethod(testGeometry);
  testMethod(testNormals);
  testMethod(testMasks);
  testMethod(testRelativePositions);
  testMethod(testInteriorFaceWithinPatch);
}

void peano4::dataview::tests::FaceMarkerTest::testSelection() {
  auto marker = createMarker();
  validateEquals(marker.getSelectedFaceNumber(), -1);
  marker.select(1);
  validateEquals(marker.getSelectedFaceNumber(), 1);
  validateEquals(marker.select(DIMENSIONS).getSelectedFaceNumber(), DIMENSIONS);
}

void peano4::dataview::tests::FaceMarkerTest::testGeometry() {
  auto marker = createMarker();

  // Face 0 sits on the negative side along the first axis, face DIMENSIONS on
  // the positive one; the other coordinates stay at the cell centre.
  validateNumericalEquals(marker.getX(0)(0), 0.5 - 1.0 / 6.0);
  validateNumericalEquals(marker.getX(DIMENSIONS)(0), 0.5 + 1.0 / 6.0);
  for (int d = 1; d < DIMENSIONS; d++) {
    validateNumericalEquals(marker.getX(0)(d), 0.5);
    validateNumericalEquals(marker.getX(DIMENSIONS)(d), 0.5);
  }

  marker.select(0);
  validateNumericalEquals(marker.getX()(0), 0.5 - 1.0 / 6.0);

  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(marker.getH()(d), 1.0 / 3.0);
  }
}

void peano4::dataview::tests::FaceMarkerTest::testNormals() {
  auto marker = createMarker();

  validateNumericalEquals(marker.normal(0)(0), -1.0 / 3.0);
  validateNumericalEquals(marker.normal(DIMENSIONS)(0), 1.0 / 3.0);
  for (int d = 1; d < DIMENSIONS; d++) {
    validateNumericalEquals(marker.normal(0)(d), 0.0);
  }
  marker.select(0);
  validateNumericalEquals(marker.normal()(0), -1.0 / 3.0);

  // A local cell keeps the cell-centric orientation; a remote cell flips it,
  // so the outer normal always points away from the local domain.
  validateNumericalEquals(marker.outerNormal(0)(0), -1.0 / 3.0);
  marker.cellIsLocal = false;
  validateNumericalEquals(marker.outerNormal(0)(0), 1.0 / 3.0);
  validateNumericalEquals(marker.outerNormal()(0), 1.0 / 3.0);
}

void peano4::dataview::tests::FaceMarkerTest::testMasks() {
  auto marker = createMarker();

  marker.hasBeenRefinedMask = 1U << 1U;
  marker.willBeRefinedMask  = 1U << 0U;
  marker.isLocalMask        = 1U << 1U;
  marker.isHangingMask      = 1U << 0U;

  validate(marker.hasBeenRefined(1));
  validateFalse(marker.hasBeenRefined(0));
  validate(marker.willBeRefined(0));
  validateFalse(marker.willBeRefined(1));
  validate(marker.isLocal(1));
  validateFalse(marker.isLocal(0));
  validate(marker.isHanging(0));
  validateFalse(marker.isHanging(1));

  // The cell state is independent of the per-face masks.
  marker.cellHasBeenRefined = true;
  marker.cellWillBeRefined  = false;
  validate(marker.hasCellBeenRefined());
  validateFalse(marker.willCellBeRefined());
  validateFalse(marker.hasBeenRefined(0));

  marker.select(1);
  validate(marker.hasBeenRefined());
  validateFalse(marker.willBeRefined());
  validate(marker.isLocal());
  validateFalse(marker.isHanging());
}

void peano4::dataview::tests::FaceMarkerTest::testRelativePositions() {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  auto          marker            = createMarker();
  for (int d = 0; d < DIMENSIONS; d++) {
    marker.relativePositionWithinFatherCell[d] = d == 0 ? SubdivisionFactor - 1 : 1;
  }

  // The face on the positive side along an axis shifts the position entry of
  // that axis by one.
  validateEquals(marker.getRelativePositionWithinFatherCell(0)(0), SubdivisionFactor - 1);
  validateEquals(marker.getRelativePositionWithinFatherCell(DIMENSIONS)(0), SubdivisionFactor);

  marker.select(0);
  validateEquals(marker.getRelativePositionWithinFatherCell()(0), SubdivisionFactor - 1);

  // The father-face position drops the normal direction's entry.
  marker.select(0);
  validateEquals(marker.getRelativePositionWithinFatherFace()(0), 1);
  marker.select(1);
  validateEquals(marker.getRelativePositionWithinFatherFace()(0), SubdivisionFactor - 1);
}

void peano4::dataview::tests::FaceMarkerTest::testInteriorFaceWithinPatch() {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  auto          marker            = createMarker();

  // Only the lower face of the first subcell and the upper face of the last
  // subcell along an axis lie on the patch boundary.
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    for (int position = 0; position < SubdivisionFactor; position++) {
      for (int d = 0; d < DIMENSIONS; d++) {
        marker.relativePositionWithinFatherCell[d] = d == normal ? position : 0;
      }
      const bool lowerFaceIsInterior = position > 0;
      const bool upperFaceIsInterior = position < SubdivisionFactor - 1;
      validateEqualsWithParams2(
        marker.select(normal).isInteriorFaceWithinPatch(),
        lowerFaceIsInterior,
        normal,
        position
      );
      validateEqualsWithParams2(
        marker.select(normal + DIMENSIONS).isInteriorFaceWithinPatch(),
        upperFaceIsInterior,
        normal,
        position
      );
    }
  }
}
