// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellErrorIntegralTest.h"

#include "exahype2/dg/ader/CellErrorIntegral.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int NodesPerAxisIn3D           = (DIMENSIONS > 2) ? NumberOfDofsPerAxis : 1;
  constexpr int NodesPerCell               = NumberOfDofsPerAxis * NodesPerAxisIn3D * NumberOfDofsPerAxis;

  constexpr double Offset       = 0.25;
  constexpr double TimeStamp    = 0.5;
  constexpr double TimeStepSize = 0.125;

  // Gauss-Legendre nodes and weights of three points on the unit interval.
  const double NodeOffsetFromCentre                   = 0.5 * std::sqrt(0.6);
  const double QuadraturePoints[NumberOfDofsPerAxis]  = {0.5 - NodeOffsetFromCentre, 0.5, 0.5 + NodeOffsetFromCentre};
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  tarch::la::Vector<DIMENSIONS, double> makeCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  double cellVolumeOf(const tarch::la::Vector<DIMENSIONS, double>& cellSize) {
    double cellVolume = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellVolume *= cellSize(axis);
    }
    return cellVolume;
  }

  struct MirrorSolution final {
    void operator()(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& nodePosition,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const double* NOALIAS const                                   numericalSolution,
      double* NOALIAS const                                         exactSolution
    ) const {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        exactSolution[unknown] = numericalSolution[unknown];
      }
    }
  };

  struct OffsetSolution final {
    void operator()(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& nodePosition,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const double* NOALIAS const                                   numericalSolution,
      double* NOALIAS const                                         exactSolution
    ) const {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        exactSolution[unknown] = numericalSolution[unknown] + Offset;
      }
    }
  };

  struct SkewedSolution final {
    void operator()(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& nodePosition,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const double* NOALIAS const                                   numericalSolution,
      double* NOALIAS const                                         exactSolution
    ) const {
      exactSolution[0] = numericalSolution[0] + 100.0;
      for (int unknown = 1; unknown < NumberOfUnknowns; unknown++) {
        exactSolution[unknown] = numericalSolution[unknown] + Offset;
      }
    }
  };

  struct ZeroSolution final {
    void operator()(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& nodePosition,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const double* NOALIAS const                  numericalSolution,
      double* NOALIAS const                                         exactSolution
    ) const {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        exactSolution[unknown] = 0.0;
      }
    }
  };

  struct RecordingSolution final {
    std::vector<double>* visitedCoordinates{nullptr};
    std::vector<double>* visitedStates{nullptr};

    void operator()(
      const tarch::la::Vector<DIMENSIONS, double>& nodePosition,
      [[maybe_unused]] const double                timeStamp,
      [[maybe_unused]] const double                timeStepSize,
      const double* NOALIAS const                  numericalSolution,
      double* NOALIAS const                        exactSolution
    ) const {
      visitedCoordinates->push_back(nodePosition(0));
      visitedStates->push_back(numericalSolution[0]);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        exactSolution[unknown] = numericalSolution[unknown];
      }
    }
  };
} // namespace

exahype2::dg::ader::tests::CellErrorIntegralTest::CellErrorIntegralTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::CellErrorIntegralTest") {}

void exahype2::dg::ader::tests::CellErrorIntegralTest::run() {
  testMethod(testAnExactSolutionLeavesNoError);
  testMethod(testAConstantOffsetWeighsTheCellVolume);
  testMethod(testOnlyTheMeasuredUnknownsCount);
  testMethod(testTheMaximumNormPicksTheLargestNode);
  testMethod(testEveryNodeIsVisitedOnceInOrder);
}

void exahype2::dg::ader::tests::CellErrorIntegralTest::testAnExactSolutionLeavesNoError() {
  double cellState[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    cellState[entry] = 1.0 + 0.125 * entry;
  }
  const int                                   measuredUnknowns[NumberOfUnknowns] = {0, 1};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const auto                                  cellSize  = makeCellSize();
  double                                      errors[3] = {7.0, 8.0, 9.0};

  exahype2::dg::ader::
    accumulateCellError<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, NumberOfUnknowns, double>(
      MirrorSolution{},
      cellState,
      QuadraturePoints,
      QuadratureWeights,
      measuredUnknowns,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize,
      errors
    );

  validateNumericalEquals(errors[0], 0.0);
  validateNumericalEquals(errors[1], 0.0);
  validateNumericalEquals(errors[2], 0.0);
}

void exahype2::dg::ader::tests::CellErrorIntegralTest::testAConstantOffsetWeighsTheCellVolume() {
  const double                                cellState[NodesPerCell * NumberOfData] = {};
  const int                                   measuredUnknowns[NumberOfUnknowns]     = {0, 1};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const auto                                  cellSize  = makeCellSize();
  double                                      errors[3] = {};

  exahype2::dg::ader::
    accumulateCellError<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, NumberOfUnknowns, double>(
      OffsetSolution{},
      cellState,
      QuadraturePoints,
      QuadratureWeights,
      measuredUnknowns,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize,
      errors
    );

  // The quadrature weights add up to one along every axis, so the weights of all nodes add up to the
  // cell volume, once per measured unknown.
  const double cellVolume = cellVolumeOf(cellSize);

  validateNumericalEquals(errors[0], NumberOfUnknowns * cellVolume * Offset);
  validateNumericalEquals(errors[1], NumberOfUnknowns * cellVolume * Offset * Offset);
  validateNumericalEquals(errors[2], Offset);
}

void exahype2::dg::ader::tests::CellErrorIntegralTest::testOnlyTheMeasuredUnknownsCount() {
  const double                                cellState[NodesPerCell * NumberOfData] = {};
  const int                                   measuredUnknowns[1]                    = {1};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const auto                                  cellSize  = makeCellSize();
  double                                      errors[3] = {};

  exahype2::dg::ader::accumulateCellError<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, 1, double>(
    SkewedSolution{},
    cellState,
    QuadraturePoints,
    QuadratureWeights,
    measuredUnknowns,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    errors
  );

  const double cellVolume = cellVolumeOf(cellSize);

  validateNumericalEquals(errors[0], cellVolume * Offset);
  validateNumericalEquals(errors[1], cellVolume * Offset * Offset);
  validateNumericalEquals(errors[2], Offset);
}

void exahype2::dg::ader::tests::CellErrorIntegralTest::testTheMaximumNormPicksTheLargestNode() {
  constexpr double LargestDifference = 5.0;

  double cellState[NodesPerCell * NumberOfData] = {};
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    const int firstValue  = cellNode * NumberOfData;
    cellState[firstValue] = 0.5;
  }
  cellState[(NodesPerCell - 1) * NumberOfData + 1] = LargestDifference;

  const int                                   measuredUnknowns[NumberOfUnknowns] = {0, 1};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const auto                                  cellSize  = makeCellSize();
  double                                      errors[3] = {};

  exahype2::dg::ader::
    accumulateCellError<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, NumberOfUnknowns, double>(
      ZeroSolution{},
      cellState,
      QuadraturePoints,
      QuadratureWeights,
      measuredUnknowns,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize,
      errors
    );

  validateNumericalEquals(errors[2], LargestDifference);
}

void exahype2::dg::ader::tests::CellErrorIntegralTest::testEveryNodeIsVisitedOnceInOrder() {
  double cellState[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    cellState[entry] = 1.0 + 0.125 * entry;
  }
  const int                                   measuredUnknowns[NumberOfUnknowns] = {0, 1};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const auto                                  cellSize  = makeCellSize();
  double                                      errors[3] = {};

  std::vector<double> visitedCoordinates;
  std::vector<double> visitedStates;

  exahype2::dg::ader::
    accumulateCellError<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, NumberOfUnknowns, double>(
      RecordingSolution{&visitedCoordinates, &visitedStates},
      cellState,
      QuadraturePoints,
      QuadratureWeights,
      measuredUnknowns,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize,
      errors
    );

  validateEquals(static_cast<int>(visitedCoordinates.size()), NodesPerCell);
  validateEquals(static_cast<int>(visitedStates.size()), NodesPerCell);
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    const double expectedCoordinate =
      cellCentre(0) - 0.5 * cellSize(0) + QuadraturePoints[cellNode % NumberOfDofsPerAxis] * cellSize(0);
    const int firstValue = cellNode * NumberOfData;
    validateNumericalEqualsWithParams1(visitedCoordinates[cellNode], expectedCoordinate, cellNode);
    validateNumericalEqualsWithParams1(visitedStates[cellNode], cellState[firstValue], cellNode);
  }
}
