// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CoordinateOwnershipTest.h"

#include "exahype2/particles/CoordinateOwnership.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr double CellWidth = 0.25;

  /** Returns an unrefined local cell of width CellWidth whose lower corner sits at lowerCorner. */
  peano4::dataview::CellMarker createCellMarker(const double lowerCorner) {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(lowerCorner + 0.5 * CellWidth);
    marker.flags  = peano4::dataview::CellMarker::IsLocal | peano4::dataview::CellMarker::IsParentLocal;
    return marker;
  }
} // namespace

exahype2::particles::tests::CoordinateOwnershipTest::CoordinateOwnershipTest() :
  tarch::tests::TestCase("exahype2::particles::tests::CoordinateOwnershipTest") {}

void exahype2::particles::tests::CoordinateOwnershipTest::run() {
  testMethod(testTheLowerCellOwnsACoordinateBetweenTwoCells);
  testMethod(testTheCellOnTheLowerDomainWallOwnsTheWall);
}

/** Of two cells that both contain the corner between them, the lower cell alone owns it. */
void exahype2::particles::tests::CoordinateOwnershipTest::testTheLowerCellOwnsACoordinateBetweenTwoCells() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(0.0);
  const auto                                  lowerCell = createCellMarker(CellWidth);
  const auto                                  upperCell = createCellMarker(2.0 * CellWidth);
  const tarch::la::Vector<DIMENSIONS, double> cornerBetweenTheCells(2.0 * CellWidth);
  const tarch::la::Vector<DIMENSIONS, double> insideTheUpperCell(2.5 * CellWidth);

  validateTrue(lowerCell.isContained(cornerBetweenTheCells));
  validateTrue(upperCell.isContained(cornerBetweenTheCells));
  validateTrue(exahype2::particles::ownsCoordinate(lowerCell, cornerBetweenTheCells, domainOffset));
  validateFalse(exahype2::particles::ownsCoordinate(upperCell, cornerBetweenTheCells, domainOffset));
  validateFalse(exahype2::particles::ownsCoordinate(lowerCell, insideTheUpperCell, domainOffset));
  validateTrue(exahype2::particles::ownsCoordinate(upperCell, insideTheUpperCell, domainOffset));
}

/** A cell owns a coordinate on its lower faces only where these faces lie on the domain wall. */
void exahype2::particles::tests::CoordinateOwnershipTest::testTheCellOnTheLowerDomainWallOwnsTheWall() {
  const auto                                  cell = createCellMarker(0.0);
  const tarch::la::Vector<DIMENSIONS, double> lowerCorner(0.0);

  validateTrue(exahype2::particles::ownsCoordinate(cell, lowerCorner, tarch::la::Vector<DIMENSIONS, double>(0.0)));
  validateFalse(exahype2::particles::ownsCoordinate(cell, lowerCorner, tarch::la::Vector<DIMENSIONS, double>(-1.0)));
}
