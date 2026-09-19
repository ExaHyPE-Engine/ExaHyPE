// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PointSourcesTest.h"

#include "exahype2/dg/ader/PointSources.h"

namespace {
  constexpr int NumberOfPointSources = 3;

  double pointSourceLocations[NumberOfPointSources][DIMENSIONS]{};

  void placePointSource(const int pointSource, const tarch::la::Vector<DIMENSIONS, double>& location) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      pointSourceLocations[pointSource][axis] = location(axis);
    }
  }

  int selectInCellOfUnitSize(
    int (&pointSourceIndices)[NumberOfPointSources],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre
  ) {
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    return exahype2::dg::ader::selectPointSources<NumberOfPointSources>(
      pointSourceIndices,
      &pointSourceLocations[0][0],
      cellCentre,
      cellSize
    );
  }
} // namespace

exahype2::dg::ader::tests::PointSourcesTest::PointSourcesTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PointSourcesTest") {}

void exahype2::dg::ader::tests::PointSourcesTest::run() {
  testMethod(testASourceInsideIsSelected);
  testMethod(testASourceOutsideIsRejected);
  testMethod(testASourceOnAFaceBelongsToBothCells);
  testMethod(testSelectionKeepsTheTableOrder);
}

void exahype2::dg::ader::tests::PointSourcesTest::testASourceInsideIsSelected() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
  placePointSource(0, cellCentre);
  placePointSource(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
  placePointSource(2, tarch::la::Vector<DIMENSIONS, double>(-5.0));

  int       pointSourceIndices[NumberOfPointSources] = {-1, -1, -1};
  const int count                                    = selectInCellOfUnitSize(pointSourceIndices, cellCentre);

  validateEquals(count, 1);
  validateEquals(pointSourceIndices[0], 0);
  validateEquals(pointSourceIndices[1], -1);
  validateEquals(pointSourceIndices[2], -1);
}

void exahype2::dg::ader::tests::PointSourcesTest::testASourceOutsideIsRejected() {
  tarch::la::Vector<DIMENSIONS, double> beyondTheLastAxis(0.5);
  beyondTheLastAxis(DIMENSIONS - 1) = 1.5;
  placePointSource(0, beyondTheLastAxis);
  placePointSource(1, tarch::la::Vector<DIMENSIONS, double>(-0.5));
  placePointSource(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

  int       pointSourceIndices[NumberOfPointSources] = {-1, -1, -1};
  const int count = selectInCellOfUnitSize(pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

  validateEquals(count, 0);
  for (int entry = 0; entry < NumberOfPointSources; entry++) {
    validateEqualsWithParams1(pointSourceIndices[entry], -1, entry);
  }
}

void exahype2::dg::ader::tests::PointSourcesTest::testASourceOnAFaceBelongsToBothCells() {
  placePointSource(0, tarch::la::Vector<DIMENSIONS, double>(0.0));
  placePointSource(1, tarch::la::Vector<DIMENSIONS, double>(1.0));
  placePointSource(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

  int       lowerIndices[NumberOfPointSources] = {-1, -1, -1};
  const int lowerCount = selectInCellOfUnitSize(lowerIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));
  int       upperIndices[NumberOfPointSources] = {-1, -1, -1};
  const int upperCount = selectInCellOfUnitSize(upperIndices, tarch::la::Vector<DIMENSIONS, double>(1.5));

  validateEquals(lowerCount, 2);
  validateEquals(lowerIndices[0], 0);
  validateEquals(lowerIndices[1], 1);
  validateEquals(upperCount, 1);
  validateEquals(upperIndices[0], 1);
}

void exahype2::dg::ader::tests::PointSourcesTest::testSelectionKeepsTheTableOrder() {
  placePointSource(0, tarch::la::Vector<DIMENSIONS, double>(0.25));
  placePointSource(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
  placePointSource(2, tarch::la::Vector<DIMENSIONS, double>(0.75));

  int       pointSourceIndices[NumberOfPointSources] = {-1, -1, -1};
  const int count = selectInCellOfUnitSize(pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

  validateEquals(count, 2);
  validateEquals(pointSourceIndices[0], 0);
  validateEquals(pointSourceIndices[1], 2);
}
