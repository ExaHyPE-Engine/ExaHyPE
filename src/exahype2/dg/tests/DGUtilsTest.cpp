// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGUtilsTest.h"

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/tests/TestUtils.h"
#include "peano4/datamanagement/CellMarker.h"

exahype2::dg::tests::DGUtilsTest::DGUtilsTest() :
  TestCase("exahype2::dg::tests::DGUtilsTest") {}

void exahype2::dg::tests::DGUtilsTest::run() {
  testMethod(testGetIndex);
  testMethod(testComputeGradientOnConstantSolution);
  testMethod(testEvaluatePolynomialOrder1);
  testMethod(testEvaluatePolynomialOrder2);
}

void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder1() {
  const int order = 1;

  peano4::datamanagement::CellMarker cellMarker;

#if DIMENSIONS == 2
  cellMarker.h                                      = {0.2, 0.2};
  cellMarker.centre                                 = {0.4, 0.6};
  double                                Q0[]        = {1.2, 1.2, 1.2, 1.2};
  tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.5, 0.7};
  tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.4 + 0.2 * QuadratureNodes1DP1[0],
    0.6 + 0.2 * QuadratureNodes1DP1[0]
  };
#else
  cellMarker.h                                      = {0.2, 0.2, 0.2};
  cellMarker.centre                                 = {0.4, 0.6, 0.8};
  double                                Q0[]        = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.5, 0.7, 0.9};
  tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.4 + 0.2 * QuadratureNodes1DP1[0],
    0.6 + 0.2 * QuadratureNodes1DP1[0],
    0.8 + 0.2 * QuadratureNodes1DP1[0]
  };
#endif

  double result;
  result = exahype2::dg::evaluatePolynomial(cellMarker, order, QuadratureNodes1DP1, 1, Q0, xLeftBottom, 0);
  validateNumericalEqualsWithParams1(result, 1.2, cellMarker.toString());

  result = exahype2::dg::evaluatePolynomial(cellMarker, order, QuadratureNodes1DP1, 1, Q0, xCentre, 0);
  validateNumericalEqualsWithParams1(result, 1.2, cellMarker.toString());
}

void exahype2::dg::tests::DGUtilsTest::testEvaluatePolynomialOrder2() {
  const int order = 2;

  peano4::datamanagement::CellMarker cellMarker;

#if DIMENSIONS == 2
  cellMarker.h                                      = {0.2, 0.2};
  cellMarker.centre                                 = {0.4, 0.6};
  double                                Q0[]        = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  tarch::la::Vector<DIMENSIONS, double> xCentre     = {0.5, 0.7};
  tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.4 + 0.2 * QuadratureNodes1DP2[0],
    0.6 + 0.2 * QuadratureNodes1DP2[0]
  };
#else
  cellMarker.h                                  = {0.2, 0.2, 0.2};
  cellMarker.centre                             = {0.4, 0.6, 0.8};
  double                                Q0[]    = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2,
                                                   1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  tarch::la::Vector<DIMENSIONS, double> xCentre = {0.5, 0.7, 0.9};
  tarch::la::Vector<DIMENSIONS, double> xLeftBottom = {
    0.4 + 0.2 * QuadratureNodes1DP1[0],
    0.6 + 0.2 * QuadratureNodes1DP1[0],
    0.8 + 0.2 * QuadratureNodes1DP1[0]
  };
#endif

  double result;
  result = exahype2::dg::evaluatePolynomial(cellMarker, order, QuadratureNodes1DP2, 1, Q0, xLeftBottom, 0);
  validateNumericalEqualsWithParams1(result, 1.2, cellMarker.toString());

  result = exahype2::dg::evaluatePolynomial(cellMarker, order, QuadratureNodes1DP2, 1, Q0, xCentre, 0);
  validateNumericalEqualsWithParams1(result, 1.2, cellMarker.toString());
}

void exahype2::dg::tests::DGUtilsTest::testGetIndex() {
  const int order        = 3;
  const int nodesPerAxis = order + 1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  validateEquals(nodesPerCell, std::pow(nodesPerAxis, DIMENSIONS));

  tarch::la::Vector<DIMENSIONS, int> strides = getStrides(nodesPerAxis);
  validateEquals(strides[DIMENSIONS - 1], std::pow(nodesPerAxis, DIMENSIONS - 1));

  tarch::la::Vector<DIMENSIONS, int> firstIndex = getIndex(0, strides);
  for (int i = 0; i < DIMENSIONS; i++) {
    validateEquals(firstIndex[i], 0);
  }

  tarch::la::Vector<DIMENSIONS, int> lastIndex = getIndex(nodesPerCell - 1, strides);
  for (int i = 0; i < DIMENSIONS; i++) {
    validateEquals(lastIndex[i], nodesPerAxis - 1);
  }
}

void exahype2::dg::tests::DGUtilsTest::testComputeGradientOnConstantSolution() {
#if DIMENSIONS == 2
  const int order        = 3;
  const int nodesPerAxis = order + 1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  double* cellQin = new double[2 * nodesPerCell];
  double  gradientValues[2 * DIMENSIONS];
  for (int i = 0; i < 2 * nodesPerCell; i++) {
    cellQin[i] = 1.0;
  }

  for (int node = 0; node < nodesPerCell; node++) {
    computeGradient(
      cellQin,
      DerivativeOperatorLagrangeP3,
      10, // invDx
      nodesPerAxis,
      2, // strideQ
      node,
      gradientValues
    );
    for (int var = 0; var < DIMENSIONS; var++) {
      validateNumericalEquals(gradientValues[var], 0.0);
    }
  }

  delete[] cellQin;
#endif
}
