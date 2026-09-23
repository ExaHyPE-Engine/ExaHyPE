// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EigenvalueStatisticsTest.h"

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/EigenvalueStatistics.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 2;
  constexpr int NumberOfUnknowns           = 1;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int CellNodes                  = exahype2::dg::NodesPerCell<NumberOfDofsPerAxis>;

  // Reports the first unknown plus the axis number plus a quarter of the first
  // coordinate, so the state, the direction and the node position all leave a
  // trace in the result.
  struct StubSolver final {
    static double maxEigenvalue(
      const double* NOALIAS const                                   state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      const int                                                     direction
    ) {
      return state[0] + direction + 0.25 * position(0);
    }
  };
} // namespace

exahype2::dg::tests::EigenvalueStatisticsTest::EigenvalueStatisticsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::EigenvalueStatisticsTest") {}

void exahype2::dg::tests::EigenvalueStatisticsTest::run() {
  testMethod(testAnisotropicAccumulation);
#if PEANO_ASSERTIONS == 0
  testMethod(testInvalidEigenvalueAccumulation);
#endif
  testMethod(testReduceMaxEigenvaluesOverTheCellNodes);
}

void exahype2::dg::tests::EigenvalueStatisticsTest::testReduceMaxEigenvaluesOverTheCellNodes() {
  const double                                quadratureNodes[NumberOfDofsPerAxis] = {0.25, 0.75};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  tarch::la::Vector<DIMENSIONS, double>       cellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellSize(axis) = 2.0 * (1 << axis);
  }

  double cellState[CellNodes * NumberOfData];
  float  narrowCellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const int firstEntry            = cellNode * NumberOfData;
    cellState[firstEntry]           = 0.5 * cellNode;
    cellState[firstEntry + 1]       = 100.0;
    narrowCellState[firstEntry]     = static_cast<float>(0.5 * cellNode);
    narrowCellState[firstEntry + 1] = 100.0F;
  }

  const auto statistics = exahype2::dg::
    reduceMaxEigenvalues<StubSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      cellState,
      quadratureNodes,
      cellCentre,
      cellSize,
      0.0,
      0.0
    );
  const auto narrowStatistics = exahype2::dg::
    reduceMaxEigenvalues<StubSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, float, double>(
      narrowCellState,
      quadratureNodes,
      cellCentre,
      cellSize,
      0.0,
      0.0
    );

  // The last node carries the largest state and the largest first coordinate,
  // so both maxima come from that node, with every axis scaled by its own extent.
  const double lastPosition   = cellCentre(0) - 0.5 * cellSize(0) + quadratureNodes[1] * cellSize(0);
  const double lastState      = 0.5 * (CellNodes - 1);
  double       expectedScaled = 0.0;
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    expectedScaled += (lastState + direction + 0.25 * lastPosition) / cellSize(direction);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, lastState + (DIMENSIONS - 1) + 0.25 * lastPosition);
  validateNumericalEquals(statistics.maxScaledEigenvalue, expectedScaled);
  validateNumericalEquals(narrowStatistics.maxPhysicalEigenvalue, statistics.maxPhysicalEigenvalue);
  validateNumericalEquals(narrowStatistics.maxScaledEigenvalue, statistics.maxScaledEigenvalue);
}

void exahype2::dg::tests::EigenvalueStatisticsTest::testAnisotropicAccumulation() {
  // Both the eigenvalues and the cell extents differ per axis, so a mix-up of the
  // axes changes the scaled maximum. The largest entry of each table lies on one
  // of the first two axes, so the physical maxima hold in two and in three dimensions.
  constexpr double FirstTable[]  = {3.0, 5.0, 2.0};
  constexpr double SecondTable[] = {7.0, 1.0, 4.0};

  tarch::la::Vector<DIMENSIONS, double> cellSize;
  std::array<double, DIMENSIONS>        firstPhysicalEigenvalues{};
  std::array<double, DIMENSIONS>        secondPhysicalEigenvalues{};
  double                                firstScaledEigenvalue  = 0.0;
  double                                secondScaledEigenvalue = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellSize(axis)                  = 2.0 * (1 << axis);
    firstPhysicalEigenvalues[axis]  = FirstTable[axis];
    secondPhysicalEigenvalues[axis] = SecondTable[axis];
    firstScaledEigenvalue += firstPhysicalEigenvalues[axis] / cellSize(axis);
    secondScaledEigenvalue += secondPhysicalEigenvalues[axis] / cellSize(axis);
  }

  exahype2::dg::MaxEigenvalueStatistics statistics;
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);

  statistics.accumulate(cellSize, [&](const int dimension) { return firstPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 5.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, firstScaledEigenvalue);

  statistics.accumulate(cellSize, [&](const int dimension) { return secondPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 7.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, secondScaledEigenvalue);

  exahype2::dg::MaxEigenvalueStatistics stationaryStatistics;
  stationaryStatistics.accumulate(cellSize, []([[maybe_unused]] const int dimension) { return 0.0; });
  validateNumericalEquals(stationaryStatistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(stationaryStatistics.maxScaledEigenvalue, 0.0);
}

void exahype2::dg::tests::EigenvalueStatisticsTest::testInvalidEigenvalueAccumulation() {
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::MaxEigenvalueStatistics negativeStatistics;
  negativeStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? -2.0 : 5.0; });
  validateEquals(negativeStatistics.maxPhysicalEigenvalue, -2.0);
  validateEquals(negativeStatistics.maxScaledEigenvalue, -2.0);

  constexpr auto                        PositiveInfinity = std::numeric_limits<double>::infinity();
  exahype2::dg::MaxEigenvalueStatistics infiniteStatistics;
  infiniteStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? PositiveInfinity : 5.0; });
  validateEquals(infiniteStatistics.maxPhysicalEigenvalue, PositiveInfinity);
  validateEquals(infiniteStatistics.maxScaledEigenvalue, PositiveInfinity);
}
