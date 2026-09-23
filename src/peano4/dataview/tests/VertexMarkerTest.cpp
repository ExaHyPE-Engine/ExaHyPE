// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexMarkerTest.h"

#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/tests/TestMacros.h"

namespace {
  peano4::grid::GridTraversalEvent createEvent() {
    peano4::grid::GridTraversalEvent            event;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0 / 3.0);
    event.setX(centre);
    event.setH(width);
    return event;
  }
} // namespace

peano4::dataview::tests::VertexMarkerTest::VertexMarkerTest() :
  TestCase("peano4::dataview::tests::VertexMarkerTest") {}

void peano4::dataview::tests::VertexMarkerTest::run() {
  testMethod(testSelection);
  testMethod(testGeometry);
  testMethod(testStateBits);
  testMethod(testRelativePositionWithinFatherCell);
  testMethod(testCoincidesWithCoarseGridVertex);
  testMethod(testContainment);
  testMethod(testAdjacency);
  testMethod(testReconstructXOfParentVertex);
}

void peano4::dataview::tests::VertexMarkerTest::testSelection() {
  auto marker = peano4::dataview::VertexMarker(createEvent());
  validateEquals(marker.getSelectedVertexNumber(), 0);
  marker.select(peano4::VerticesPerCell - 1);
  validateEquals(marker.getSelectedVertexNumber(), peano4::VerticesPerCell - 1);
  validateEquals(peano4::dataview::VertexMarker(createEvent(), 1).getSelectedVertexNumber(), 1);
}

void peano4::dataview::tests::VertexMarkerTest::testGeometry() {
  const auto marker = peano4::dataview::VertexMarker(createEvent());

  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(marker.getH()(d), 1.0 / 3.0);
    validateNumericalEquals(marker.getInvokingCellCentre()(d), 0.5);
    // Vertex 0 is the cell's low corner, the last one the high corner.
    validateNumericalEquals(marker.getX(0)(d), 0.5 - 1.0 / 6.0);
    validateNumericalEquals(marker.getX(peano4::VerticesPerCell - 1)(d), 0.5 + 1.0 / 6.0);
    validateNumericalEquals(marker.getX()(d), 0.5 - 1.0 / 6.0);
  }
}

void peano4::dataview::tests::VertexMarkerTest::testStateBits() {
  auto event = createEvent();
  event.setHasBeenRefined(0, true);
  event.setWillBeRefined(1, true);
  event.setIsVertexLocal(0, true);
  event.setIsVertexHanging(1, true);
  event.setIsVertexParentOfSubtree(0, true);
  event.setIsVertexAdjacentToParallelDomainBoundary(1, true);
  event.setIsParentCellLocal(true);
  event.setIsParentVertexLocal(0, true);

  auto marker = peano4::dataview::VertexMarker(event);

  validate(marker.hasBeenRefined(0));
  validateFalse(marker.hasBeenRefined(1));
  validate(marker.willBeRefined(1));
  validateFalse(marker.willBeRefined(0));
  validate(marker.isLocal(0));
  validateFalse(marker.isLocal(1));
  validate(marker.isHanging(1));
  validateFalse(marker.isHanging(0));
  validate(marker.isParentOfSubtree(0));
  validateFalse(marker.isParentOfSubtree(1));
  validate(marker.isAdjacentToParallelDomainBoundary(1));
  validateFalse(marker.isAdjacentToParallelDomainBoundary(0));
  validate(marker.isParentCellLocal());
  validate(marker.isParentVertexLocal(0));
  validateFalse(marker.isParentVertexLocal(1));

  // The selected vertex feeds the parameterless accessors.
  validate(marker.hasBeenRefined());
  validateFalse(marker.isHanging());
  marker.select(1);
  validateFalse(marker.hasBeenRefined());
  validate(marker.isHanging());
  validate(marker.isAdjacentToParallelDomainBoundary());
}

void peano4::dataview::tests::VertexMarkerTest::testRelativePositionWithinFatherCell() {
  auto                                     event = createEvent();
  const tarch::la::Vector<DIMENSIONS, int> position(1);
  event.setRelativePositionToFather(position);

  const auto marker = peano4::dataview::VertexMarker(event);

  // Vertex i adds one to the cell position along every axis where its index
  // bit is set.
  validateEquals(marker.getRelativePositionWithinFatherCell(0)(0), 1);
  validateEquals(marker.getRelativePositionWithinFatherCell(1)(0), 2);
  validateEquals(marker.getRelativePositionWithinFatherCell()(0), 1);
}

void peano4::dataview::tests::VertexMarkerTest::testCoincidesWithCoarseGridVertex() {
  auto                                     event = createEvent();
  const tarch::la::Vector<DIMENSIONS, int> cornerPosition(0);
  event.setRelativePositionToFather(cornerPosition);

  auto marker = peano4::dataview::VertexMarker(event);
  validate(marker.coincidesWithCoarseGridVertex());

  const tarch::la::Vector<DIMENSIONS, int> innerPosition(1);
  event.setRelativePositionToFather(innerPosition);
  marker = peano4::dataview::VertexMarker(event);
  validateFalse(marker.coincidesWithCoarseGridVertex());
}

void peano4::dataview::tests::VertexMarkerTest::testContainment() {
  const auto marker = peano4::dataview::VertexMarker(createEvent());

  // The selected vertex sits at the cell's low corner; its adjacent cells
  // reach one cell width in every direction.
  const tarch::la::Vector<DIMENSIONS, double> nearPoint(0.5 - 1.0 / 6.0 + 0.1);
  validate(marker.isContainedInAdjacentCells(nearPoint));
  validate(marker.isContainedInAdjacentCellsByDimension(nearPoint, 0));

  tarch::la::Vector<DIMENSIONS, double> farPoint(0.5 - 1.0 / 6.0);
  farPoint[0] += 0.5;
  validateFalse(marker.isContainedInAdjacentCells(farPoint));
  validateFalse(marker.isContainedInAdjacentCellsByDimension(farPoint, 0));
  validate(marker.isContainedInAdjacentCells(farPoint, 2.0));

  validateEquals(marker.getRelativeAdjacentCell(nearPoint)(0), 1);
  const tarch::la::Vector<DIMENSIONS, double> belowPoint(0.5 - 1.0 / 6.0 - 0.1);
  validateEquals(marker.getRelativeAdjacentCell(belowPoint)(0), 0);
}

void peano4::dataview::tests::VertexMarkerTest::testAdjacency() {
  auto event = createEvent();
  // Mark every adjacent cell of the 3^d neighbourhood as local.
  for (int adjacentCell = 0; adjacentCell < peano4::CellsPerNeighbourhood; adjacentCell++) {
    event.setIsAdjacentCellLocal(adjacentCell, true);
  }
  auto marker = peano4::dataview::VertexMarker(event);
  validateEquals(static_cast<int>(marker.areAdjacentCellsLocal().count()), peano4::CellsPerVertex);

  event.setIsAdjacentCellLocal(0U);
  marker = peano4::dataview::VertexMarker(event);
  validateEquals(static_cast<int>(marker.areAdjacentCellsLocal().count()), 0);
}

void peano4::dataview::tests::VertexMarkerTest::testReconstructXOfParentVertex() {
  auto                                     event = createEvent();
  const tarch::la::Vector<DIMENSIONS, int> cornerPosition(0);
  event.setRelativePositionToFather(cornerPosition);

  const auto marker = peano4::dataview::VertexMarker(event);

  // At the parent's low corner, parent vertex 0 coincides with the selected
  // vertex and the opposite parent vertex sits one parent cell width away.
  const auto lowCorner = peano4::dataview::reconstructXOfParentVertex(marker, std::bitset<DIMENSIONS>(0));
  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(lowCorner(d), marker.getX()(d));
  }

  const auto highCorner =
    peano4::dataview::reconstructXOfParentVertex(marker, std::bitset<DIMENSIONS>((1U << DIMENSIONS) - 1U));
  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(
      highCorner(d),
      marker.getX()(d) + peano4::sfc::ActiveCurve::SubdivisionFactor * marker.getH()(d)
    );
  }
}
