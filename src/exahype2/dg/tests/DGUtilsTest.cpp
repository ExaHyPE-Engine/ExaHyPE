// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGUtilsTest.h"

#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/grid/GridTraversalEvent.h"

namespace {
  constexpr double QuadratureNodes1DP1[] = {0.21132486540518713, 0.7886751345948129};
  constexpr double QuadratureNodes1DP2[] = {0.1127016653792583, 0.5, 0.8872983346207417};

  tarch::la::Vector<DIMENSIONS, double> rampOverAxes(const double first, const double increment) {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = first + increment * axis;
    }
    return result;
  }

  double affineField(const tarch::la::Vector<DIMENSIONS, double>& position) {
    double result = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result += (1.0 + axis) * position(axis);
    }
    return result;
  }

  // An affine field lies in the nodal basis of order one and higher, so a
  // polynomial that interpolates it returns it at every point of the cell.
  void fillAffineNodalValues(
    const peano4::dataview::CellMarker& cellMarker,
    const int                           order,
    const double* NOALIAS const         nodes,
    double* NOALIAS const               values
  ) {
    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      tarch::la::Vector<DIMENSIONS, double> position;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        position(axis) = cellMarker.getOffset()(axis) + nodes[node(axis)] * cellMarker.getH()(axis);
      }
      values[peano4::dLinearised(node, order + 1)] = affineField(position);
    });
  }
} // namespace

exahype2::dg::tests::DGUtilsTest::DGUtilsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::DGUtilsTest") {}

void exahype2::dg::tests::DGUtilsTest::run() {
  testMethod(testNodeCountsFollowTheNumberOfNodesPerAxis);
  testMethod(testGetQuadraturePointMapsTheReferenceNode);
  testMethod(testEvaluatePolynomialOrder1);
  testMethod(testEvaluatePolynomialOrder2);
  testMethod(testEvaluatePolynomialOnAFace);
}

/** A cell holds one node per combination of the nodes along the axes, and a face one per tangential combination. */
void exahype2::dg::tests::DGUtilsTest::testNodeCountsFollowTheNumberOfNodesPerAxis() {
  constexpr int NumberOfDofsPerAxis = 3;

  int nodesInCell = 0;
  peano4::dForEach(NumberOfDofsPerAxis, [&]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, int>& node) {
    nodesInCell++;
  });

  validateEquals(exahype2::dg::NodesPerCell<NumberOfDofsPerAxis>, nodesInCell);
  validateEquals(exahype2::dg::NodesPerFace<NumberOfDofsPerAxis>, nodesInCell / NumberOfDofsPerAxis);
}

/** A reference node maps onto the cell offset plus the node's fraction of the cell size. */
void exahype2::dg::tests::DGUtilsTest::testGetQuadraturePointMapsTheReferenceNode() {
  const auto                         cellCentre = rampOverAxes(0.4, 0.2);
  const auto                         cellSize   = rampOverAxes(0.2, 0.2);
  tarch::la::Vector<DIMENSIONS, int> index;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    index(axis) = (2 * axis) % 3;
  }

  const auto position = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, QuadratureNodes1DP2);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = cellCentre(axis) + (QuadratureNodes1DP2[index(axis)] - 0.5) * cellSize(axis);
    validateNumericalEqualsWithParams1(position(axis), expected, axis);
  }

  // A table in another precision is widened to double before it scales the cell.
  const float narrowNodes[]  = {0.25F, 0.5F, 0.75F};
  const auto  narrowPosition = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, narrowNodes);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = cellCentre(axis) + (static_cast<double>(narrowNodes[index(axis)]) - 0.5) * cellSize(axis);
    validateNumericalEqualsWithParams1(narrowPosition(axis), expected, axis);
  }
}

/** A polynomial of order one returns the affine field it interpolates at every point of the cell. */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder1() {
  constexpr int Order        = 1;
  constexpr int NodesPerCell = (DIMENSIONS == 3) ? (Order + 1) * (Order + 1) * (Order + 1) : (Order + 1) * (Order + 1);

  peano4::dataview::CellMarker cellMarker;
  cellMarker.h      = tarch::la::Vector<DIMENSIONS, double>(0.2);
  cellMarker.centre = rampOverAxes(0.4, 0.2);

  double values[NodesPerCell];
  fillAffineNodalValues(cellMarker, Order, QuadratureNodes1DP1, values);

  auto firstNode = cellMarker.getOffset();
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    firstNode(axis) += QuadratureNodes1DP1[0] * cellMarker.getH()(axis);
  }
  const double valueAtFirstNode =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP1, 1, values, firstNode, 0);
  validateNumericalEqualsWithParams1(valueAtFirstNode, affineField(firstNode), cellMarker.toString());

  const auto   centre = cellMarker.getX();
  const double valueAtCentre =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP1, 1, values, centre, 0);
  validateNumericalEqualsWithParams1(valueAtCentre, affineField(centre), cellMarker.toString());
}

/** A polynomial of order two returns the affine field it interpolates at every point of the cell. */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder2() {
  constexpr int Order        = 2;
  constexpr int NodesPerCell = (DIMENSIONS == 3) ? (Order + 1) * (Order + 1) * (Order + 1) : (Order + 1) * (Order + 1);

  peano4::dataview::CellMarker cellMarker;
  cellMarker.h      = tarch::la::Vector<DIMENSIONS, double>(0.2);
  cellMarker.centre = rampOverAxes(0.4, 0.2);

  double values[NodesPerCell];
  fillAffineNodalValues(cellMarker, Order, QuadratureNodes1DP2, values);

  auto firstNode = cellMarker.getOffset();
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    firstNode(axis) += QuadratureNodes1DP2[0] * cellMarker.getH()(axis);
  }
  const double valueAtFirstNode =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP2, 1, values, firstNode, 0);
  validateNumericalEqualsWithParams1(valueAtFirstNode, affineField(firstNode), cellMarker.toString());

  const auto   centre = cellMarker.getX();
  const double valueAtCentre =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP2, 1, values, centre, 0);
  validateNumericalEqualsWithParams1(valueAtCentre, affineField(centre), cellMarker.toString());
}

/**
 * A profile that is linear along every tangential axis is reproduced at a
 * position between the face nodes.
 */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOnAFace() {
  constexpr int Order           = 1;
  constexpr int FaceNumber      = 0;
  constexpr int UnknownsPerNode = 2;
  constexpr int Unknown         = 1;
  constexpr int NodesPerFace    = (DIMENSIONS == 3) ? (Order + 1) * (Order + 1) : Order + 1;

  peano4::grid::GridTraversalEvent event;
  event.setX(rampOverAxes(0.4, 0.2));
  event.setH(rampOverAxes(0.2, 0.2));
  const auto slopes            = rampOverAxes(1.0, 1.0);
  const auto referencePosition = rampOverAxes(0.5, -0.2);

  auto faceMarker = event.getFaceMarker();
  faceMarker.select(FaceNumber);

  double values[NodesPerFace * UnknownsPerNode] = {};
  int    node                                   = 0;
  peano4::dForEachExclusive(Order + 1, FaceNumber, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    double value = 1.0;
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      value += slopes(axis) * QuadratureNodes1DP1[index(axis)];
    }
    const int firstEntry         = node * UnknownsPerNode;
    values[firstEntry]           = -value;
    values[firstEntry + Unknown] = value;
    node++;
  });

  auto   position = faceMarker.getX();
  double expected = 1.0;
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    position(axis) += (referencePosition(axis) - 0.5) * faceMarker.getH()(axis);
    expected += slopes(axis) * referencePosition(axis);
  }

  const double result = exahype2::dg::evaluatePolynomial(
    faceMarker,
    Order,
    QuadratureNodes1DP1,
    UnknownsPerNode,
    values,
    position,
    Unknown
  );
  validateNumericalEqualsWithParams1(result, expected, faceMarker.toString());
}
