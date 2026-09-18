// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGUtilsTest.h"

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/tests/TestUtils.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"

exahype2::dg::tests::DGUtilsTest::DGUtilsTest() :
  TestCase("exahype2::dg::tests::DGUtilsTest") {}

void exahype2::dg::tests::DGUtilsTest::run() {
  testMethod(testGetIndex);
  testMethod(testComputeGradientOnConstantSolution);
  testMethod(testEvaluatePolynomialOrder1);
  testMethod(testEvaluatePolynomialOrder2);
  testMethod(testEvaluatePolynomialOnFace);
  testMethod(testGetQuadraturePoint);
}

/** A polynomial of order one returns the constant it interpolates at every point of the cell. */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder1() {
  constexpr int Order = 1;

  peano4::dataview::CellMarker cellMarker;

#if DIMENSIONS == 2
  cellMarker.h      = {0.2, 0.2};
  cellMarker.centre = {0.4, 0.6};

  const double                                Q0[]        = {1.2, 1.2, 1.2, 1.2};
  const tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.4, 0.6};
  const tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.3 + 0.2 * QuadratureNodes1DP1[0],
    0.5 + 0.2 * QuadratureNodes1DP1[0]
  };
#else
  cellMarker.h      = {0.2, 0.2, 0.2};
  cellMarker.centre = {0.4, 0.6, 0.8};

  const double                                Q0[]        = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  const tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.4, 0.6, 0.8};
  const tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.3 + 0.2 * QuadratureNodes1DP1[0],
    0.5 + 0.2 * QuadratureNodes1DP1[0],
    0.7 + 0.2 * QuadratureNodes1DP1[0]
  };
#endif

  const double valueAtLeftBottom =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP1, 1, Q0, xLeftBottom, 0);
  validateNumericalEqualsWithParams1(valueAtLeftBottom, 1.2, cellMarker.toString());

  const double valueAtCentre =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP1, 1, Q0, xCentre, 0);
  validateNumericalEqualsWithParams1(valueAtCentre, 1.2, cellMarker.toString());
}

/** A polynomial of order two returns the constant it interpolates at every point of the cell. */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder2() {
  constexpr int Order = 2;

  peano4::dataview::CellMarker cellMarker;

#if DIMENSIONS == 2
  cellMarker.h      = {0.2, 0.2};
  cellMarker.centre = {0.4, 0.6};

  const double                                Q0[]        = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  const tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.4, 0.6};
  const tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.3 + 0.2 * QuadratureNodes1DP2[0],
    0.5 + 0.2 * QuadratureNodes1DP2[0]
  };
#else
  cellMarker.h      = {0.2, 0.2, 0.2};
  cellMarker.centre = {0.4, 0.6, 0.8};

  const double Q0[] = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2,
                       1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  const tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.4, 0.6, 0.8};
  const tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.3 + 0.2 * QuadratureNodes1DP2[0],
    0.5 + 0.2 * QuadratureNodes1DP2[0],
    0.7 + 0.2 * QuadratureNodes1DP2[0]
  };
#endif

  const double valueAtLeftBottom =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP2, 1, Q0, xLeftBottom, 0);
  validateNumericalEqualsWithParams1(valueAtLeftBottom, 1.2, cellMarker.toString());

  const double valueAtCentre =
    exahype2::dg::evaluatePolynomial(cellMarker, Order, QuadratureNodes1DP2, 1, Q0, xCentre, 0);
  validateNumericalEqualsWithParams1(valueAtCentre, 1.2, cellMarker.toString());
}

/** The face polynomial returns the constant it interpolates and reproduces a tangential ramp. */
void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOnFace() {
  constexpr int Order          = 1;
  constexpr int LeftFaceNumber = 0;

  peano4::dataview::FaceMarker faceMarker;

#if DIMENSIONS == 2
  faceMarker.h          = {0.2, 0.2};
  faceMarker.cellCentre = {0.4, 0.6};

  const double constantQ[] = {1.2, 1.2};
#else
  faceMarker.h          = {0.2, 0.2, 0.2};
  faceMarker.cellCentre = {0.4, 0.6, 0.8};

  const double constantQ[] = {1.2, 1.2, 1.2, 1.2};
#endif

  faceMarker.select(LeftFaceNumber);

  const tarch::la::Vector<DIMENSIONS, double> faceCentre = faceMarker.getX();

  const double valueAtFaceCentre =
    exahype2::dg::evaluatePolynomial(faceMarker, Order, QuadratureNodes1DP1, 1, constantQ, faceCentre, 0);
  validateNumericalEqualsWithParams1(valueAtFaceCentre, 1.2, faceMarker.toString());

#if DIMENSIONS == 2
  // The two face nodes carry their own reference coordinate, so the polynomial returns the
  // reference coordinate of every point of the face.
  const double rampQ[] = {QuadratureNodes1DP1[0], QuadratureNodes1DP1[1]};

  const double rampAtFaceCentre =
    exahype2::dg::evaluatePolynomial(faceMarker, Order, QuadratureNodes1DP1, 1, rampQ, faceCentre, 0);
  validateNumericalEqualsWithParams1(rampAtFaceCentre, 0.5, faceMarker.toString());

  tarch::la::Vector<DIMENSIONS, double> pointOnFace = faceCentre;
  pointOnFace(1) += 0.25 * faceMarker.getH()(1);

  const double rampAtPointOnFace =
    exahype2::dg::evaluatePolynomial(faceMarker, Order, QuadratureNodes1DP1, 1, rampQ, pointOnFace, 0);
  validateNumericalEqualsWithParams1(rampAtPointOnFace, 0.75, faceMarker.toString());
#endif
}

/** The centre node of an order-two cell maps onto the cell centre and the outer nodes stay inside. */
void exahype2::dg::tests::DGUtilsTest::testGetQuadraturePoint() {
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.2);

#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> cellCentre = {0.4, 0.6};
#else
  const tarch::la::Vector<DIMENSIONS, double> cellCentre = {0.4, 0.6, 0.8};
#endif

  const tarch::la::Vector<DIMENSIONS, int> firstNode(0);
  const tarch::la::Vector<DIMENSIONS, int> centreNode(1);
  const tarch::la::Vector<DIMENSIONS, int> lastNode(2);

  const tarch::la::Vector<DIMENSIONS, double> firstPoint =
    exahype2::dg::getQuadraturePoint(cellCentre, cellSize, firstNode, QuadratureNodes1DP2);
  const tarch::la::Vector<DIMENSIONS, double> centrePoint =
    exahype2::dg::getQuadraturePoint(cellCentre, cellSize, centreNode, QuadratureNodes1DP2);
  const tarch::la::Vector<DIMENSIONS, double> lastPoint =
    exahype2::dg::getQuadraturePoint(cellCentre, cellSize, lastNode, QuadratureNodes1DP2);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEqualsWithParams1(centrePoint(axis), cellCentre(axis), axis);
    validateNumericalEqualsWithParams1(0.5 * (firstPoint(axis) + lastPoint(axis)), cellCentre(axis), axis);
    validateWithParams2(firstPoint(axis) > cellCentre(axis) - 0.5 * cellSize(axis), firstPoint(axis), axis);
    validateWithParams2(lastPoint(axis) < cellCentre(axis) + 0.5 * cellSize(axis), lastPoint(axis), axis);
  }
}

/** The strides of a cell and the index of a serialised node invert each other. */
void exahype2::dg::tests::DGUtilsTest::testGetIndex() {
  constexpr int Order        = 3;
  constexpr int NodesPerAxis = Order + 1;

  const int nodesPerCell = exahype2::dg::getNodesPerCell(NodesPerAxis);
  validateEquals(nodesPerCell, std::pow(NodesPerAxis, DIMENSIONS));

  const tarch::la::Vector<DIMENSIONS, int> strides = exahype2::dg::getStrides(NodesPerAxis);
  validateEquals(strides[DIMENSIONS - 1], std::pow(NodesPerAxis, DIMENSIONS - 1));

  const tarch::la::Vector<DIMENSIONS, int> firstIndex = exahype2::dg::getIndex(0, strides);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateEqualsWithParams1(firstIndex[axis], 0, axis);
  }

  const tarch::la::Vector<DIMENSIONS, int> lastIndex = exahype2::dg::getIndex(nodesPerCell - 1, strides);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateEqualsWithParams1(lastIndex[axis], NodesPerAxis - 1, axis);
  }
}

/** The gradient of a constant solution vanishes in every direction and at every node. */
void exahype2::dg::tests::DGUtilsTest::testComputeGradientOnConstantSolution() {
  constexpr int    Order            = 3;
  constexpr int    NodesPerAxis     = Order + 1;
  constexpr int    NodesPerCell     = NodesPerAxis * NodesPerAxis * (DIMENSIONS == 3 ? NodesPerAxis : 1);
  constexpr int    StrideQ          = 2;
  constexpr double InverseCellWidth = 10.0;

  double cellQin[NodesPerCell * StrideQ];
  for (int entry = 0; entry < NodesPerCell * StrideQ; entry++) {
    cellQin[entry] = 1.0;
  }

  double gradientValues[DIMENSIONS * StrideQ];
  for (int node = 0; node < NodesPerCell; node++) {
    exahype2::dg::computeGradient(
      cellQin,
      DerivativeOperatorLagrangeP3,
      InverseCellWidth,
      NodesPerAxis,
      StrideQ,
      node,
      gradientValues
    );

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int quantity = 0; quantity < StrideQ; quantity++) {
        validateNumericalEqualsWithParams3(gradientValues[axis * StrideQ + quantity], 0.0, node, axis, quantity);
      }
    }
  }
}
