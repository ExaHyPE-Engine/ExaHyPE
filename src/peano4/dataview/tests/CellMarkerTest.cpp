// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellMarkerTest.h"

#include "peano4/dataview/CellMarker.h"
#include "tarch/tests/TestMacros.h"

namespace {
  peano4::dataview::CellMarker createMarker() {
    peano4::dataview::CellMarker marker;
    marker.level = 2;
    for (int d = 0; d < DIMENSIONS; d++) {
      marker.h[d]      = 1.0 / 3.0;
      marker.centre[d] = 0.5;
    }
    return marker;
  }
} // namespace

peano4::dataview::tests::CellMarkerTest::CellMarkerTest() :
  TestCase("peano4::dataview::tests::CellMarkerTest") {}

void peano4::dataview::tests::CellMarkerTest::run() {
  testMethod(testFlags);
  testMethod(testGeometry);
  testMethod(testIsContained);
  testMethod(testOverlaps);
  testMethod(testInvokingParentCellsCentre);
}

void peano4::dataview::tests::CellMarkerTest::testFlags() {
  auto marker = createMarker();

  validateFalse(marker.hasBeenRefined());
  validateFalse(marker.willBeRefined());
  validateFalse(marker.isLocal());
  validateFalse(marker.isParentLocal());

  marker.flags = peano4::dataview::CellMarker::HasBeenRefined | peano4::dataview::CellMarker::IsLocal;
  validate(marker.hasBeenRefined());
  validateFalse(marker.willBeRefined());
  validate(marker.isLocal());
  validateFalse(marker.isParentLocal());

  marker.flags = peano4::dataview::CellMarker::WillBeRefined | peano4::dataview::CellMarker::IsParentLocal;
  validateFalse(marker.hasBeenRefined());
  validate(marker.willBeRefined());
  validateFalse(marker.isLocal());
  validate(marker.isParentLocal());
}

void peano4::dataview::tests::CellMarkerTest::testGeometry() {
  const auto marker = createMarker();

  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(marker.getH()(d), 1.0 / 3.0);
    validateNumericalEquals(marker.getX()(d), 0.5);
    validateNumericalEquals(marker.getOffset()(d), 0.5 - 1.0 / 6.0);
  }
}

void peano4::dataview::tests::CellMarkerTest::testIsContained() {
  const auto marker = createMarker();

  tarch::la::Vector<DIMENSIONS, double> inside(0.5);
  validate(marker.isContained(inside));

  tarch::la::Vector<DIMENSIONS, double> outside(0.5);
  outside[0] = 0.9;
  validateFalse(marker.isContained(outside));

  // A point on the cell boundary counts as contained.
  tarch::la::Vector<DIMENSIONS, double> onBoundary(0.5);
  onBoundary[0] = 0.5 + 1.0 / 6.0;
  validate(marker.isContained(onBoundary));

  validate(peano4::dataview::CellMarker::isContained(inside, marker.centre, marker.h, 0.0));
  validateFalse(peano4::dataview::CellMarker::isContained(outside, marker.centre, marker.h, 0.0));
}

void peano4::dataview::tests::CellMarkerTest::testOverlaps() {
  const auto marker = createMarker();

  tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  tarch::la::Vector<DIMENSIONS, double> size(1.0);
  validate(marker.overlaps(offset, size));

  tarch::la::Vector<DIMENSIONS, double> disjointOffset(2.0);
  validateFalse(marker.overlaps(disjointOffset, size));

  // A box that only touches the cell face still overlaps.
  tarch::la::Vector<DIMENSIONS, double> touchingOffset(0.5);
  touchingOffset[0] = 0.5 + 1.0 / 6.0;
  validate(marker.overlaps(touchingOffset, size));
}

void peano4::dataview::tests::CellMarkerTest::testInvokingParentCellsCentre() {
  auto marker = createMarker();

  for (int d = 0; d < DIMENSIONS; d++) {
    marker.relativePositionWithinFatherCell[d] = 1;
  }
  marker.relativePositionWithinFatherCell[0] = 0;

  const auto parentCentre = marker.getInvokingParentCellsCentre();
  validateNumericalEquals(parentCentre(0), 0.5 + 1.0 / 3.0);
  for (int d = 1; d < DIMENSIONS; d++) {
    validateNumericalEquals(parentCentre(d), 0.5);
  }

  marker.relativePositionWithinFatherCell[0] = 2;
  validateNumericalEquals(marker.getInvokingParentCellsCentre()(0), 0.5 - 1.0 / 3.0);
}
