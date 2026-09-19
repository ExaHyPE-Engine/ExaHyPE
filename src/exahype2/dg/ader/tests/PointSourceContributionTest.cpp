// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PointSourceContributionTest.h"

#include "exahype2/dg/ader/PointSourceContribution.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfPointSources       = 2;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int ForceStride = CellLayout::PaddedNumberOfUnknowns;
  constexpr int SampleSize  = (NumberOfDofsPerAxis + 1) * CellLayout::NodesPerCell * ForceStride;
  constexpr int AxisStride  = CellLayout::NodesPerCell * ForceStride;
  constexpr int StateSize   = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfData;

  constexpr double TimeStamp    = 0.25;
  constexpr double TimeStepSize = 0.5;

  // Gauss-Legendre nodes and weights of three points on the unit interval.
  const double NodeOffsetFromCentre                   = 0.5 * std::sqrt(0.6);
  const double QuadraturePoints[NumberOfDofsPerAxis]  = {0.5 - NodeOffsetFromCentre, 0.5, 0.5 + NodeOffsetFromCentre};
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  // Lagrange basis function of the given node evaluated at x.
  double basisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NumberOfDofsPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadraturePoints[other]) / (QuadraturePoints[node] - QuadraturePoints[other]);
      }
    }
    return result;
  }

  tarch::la::Vector<DIMENSIONS, int> nodeIndexOf(const int cellNode) {
    tarch::la::Vector<DIMENSIONS, int> nodeIndex;
    int                                remainder = cellNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
      remainder /= NumberOfDofsPerAxis;
    }
    return nodeIndex;
  }

  tarch::la::Vector<DIMENSIONS, double> makeCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  double referenceCoordinate(const int pointSource, const int axis) {
    return (pointSource == 0) ? 0.2 + 0.1 * axis : 0.7 - 0.15 * axis;
  }

  double pointSourceForce(
    const int    unknown,
    const int    pointSource,
    const double timePosition,
    const double stateValue
  ) {
    return 1.0 + unknown + pointSource + timePosition + stateValue;
  }

  // Sample zero sits at the start of the step, the following samples on the quadrature points of the step.
  double timePositionOf(const int timeSample) {
    return (timeSample == 0) ? TimeStamp : TimeStamp + TimeStepSize * QuadraturePoints[timeSample - 1];
  }

  double pointSourceLocations[NumberOfPointSources][DIMENSIONS]{};

  struct PointSourceSolver final {
    static void pointSource(
      const double* NOALIAS const                  state,
      [[maybe_unused]] const double* NOALIAS const nodePosition,
      const double                                 timeStamp,
      [[maybe_unused]] const double                timeStepSize,
      double* NOALIAS const                        forceVector,
      const int                                    pointSource
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        forceVector[unknown] = pointSourceForce(unknown, pointSource, timeStamp, state[unknown]);
      }
    }
  };

  void placePointSources(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    for (int pointSource = 0; pointSource < NumberOfPointSources; pointSource++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        pointSourceLocations[pointSource][axis] =
          cellCentre(axis) + cellSize(axis) * (referenceCoordinate(pointSource, axis) - 0.5);
      }
    }
  }

  void fillInverseCellQuadratureWeights(double (&inverseCellQuadratureWeights)[CellLayout::NodesPerCell]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = nodeIndexOf(cellNode);
      double     weight    = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        weight *= QuadratureWeights[nodeIndex(axis)];
      }
      inverseCellQuadratureWeights[cellNode] = 1.0 / weight;
    }
  }

  double seedOfState(const int entry) { return 0.25 + 0.125 * entry; }
} // namespace

exahype2::dg::ader::tests::PointSourceContributionTest::PointSourceContributionTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PointSourceContributionTest") {}

void exahype2::dg::ader::tests::PointSourceContributionTest::run() {
  testMethod(testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers);
  testMethod(testTheBasisFunctionsReproduceALinearFunction);
  testMethod(testNoPointSourceLeavesTheSamplesEmpty);
  testMethod(testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume);
  testMethod(testTheTimeIntegralSkipsTheSampleAtTheStart);
  testMethod(testTheDerivativeCoefficientsWeightTheUnknownsOnly);
  testMethod(testEveryAxisReceivesTheSameTimeAverage);
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers() {
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    for (int node = 0; node < NumberOfDofsPerAxis; node++) {
      const double value = exahype2::dg::ader::evaluateLagrangeBasis<NumberOfDofsPerAxis, double>(
        QuadraturePoints,
        index,
        QuadraturePoints[node]
      );
      const double expected = (index == node) ? 1.0 : 0.0;
      validateNumericalEqualsWithParams2(value, expected, index, node);
    }
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testTheBasisFunctionsReproduceALinearFunction() {
  for (int sample = 0; sample <= 8; sample++) {
    const double position = 0.125 * sample;
    double       constant = 0.0;
    double       linear   = 0.0;
    for (int index = 0; index < NumberOfDofsPerAxis; index++) {
      const double value =
        exahype2::dg::ader::evaluateLagrangeBasis<NumberOfDofsPerAxis, double>(QuadraturePoints, index, position);
      constant += value;
      linear += value * QuadraturePoints[index];
    }
    validateNumericalEqualsWithParams1(constant, 1.0, sample);
    validateNumericalEqualsWithParams1(linear, position, sample);
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testNoPointSourceLeavesTheSamplesEmpty() {
  const auto                                  cellSize = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  placePointSources(cellCentre, cellSize);

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
  }
  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  fillInverseCellQuadratureWeights(inverseCellQuadratureWeights);

  const double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData] = {};
  const int    pointSourceIndices[NumberOfPointSources]                       = {0, 1};

  double pointSourceValues[SampleSize];
  for (int entry = 0; entry < SampleSize; entry++) {
    pointSourceValues[entry] = 9.0;
  }

  exahype2::dg::ader::evaluatePointSources<
    PointSourceSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    double,
    double>(
    pointSourceValues,
    cellState,
    pointSourceIndices,
    0,
    &pointSourceLocations[0][0],
    QuadraturePoints,
    inverseCellQuadratureWeights,
    inverseCellSize,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < SampleSize; entry++) {
    validateNumericalEqualsWithParams1(pointSourceValues[entry], 0.0, entry);
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::
  testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume() {
  const auto                                  cellSize = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  placePointSources(cellCentre, cellSize);

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  double                                cellVolume = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
    cellVolume *= cellSize(axis);
  }
  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  fillInverseCellQuadratureWeights(inverseCellQuadratureWeights);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int entry = 0; entry < CellLayout::NodesPerCell * CellLayout::NumberOfData; entry++) {
    cellState[entry] = 0.5 + 0.01 * entry;
  }

  const int pointSourceIndices[NumberOfPointSources] = {1, 0};

  double pointSourceValues[SampleSize];
  for (int entry = 0; entry < SampleSize; entry++) {
    pointSourceValues[entry] = 9.0;
  }

  exahype2::dg::ader::evaluatePointSources<
    PointSourceSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    double,
    double>(
    pointSourceValues,
    cellState,
    pointSourceIndices,
    NumberOfPointSources,
    &pointSourceLocations[0][0],
    QuadraturePoints,
    inverseCellQuadratureWeights,
    inverseCellSize,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int timeSample = 0; timeSample < NumberOfDofsPerAxis + 1; timeSample++) {
    const double timePosition = timePositionOf(timeSample);
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = nodeIndexOf(cellNode);
      for (int unknown = 0; unknown < ForceStride; unknown++) {
        double expected = 0.0;
        if (unknown < NumberOfUnknowns) {
          for (int entry = 0; entry < NumberOfPointSources; entry++) {
            const int pointSource = pointSourceIndices[entry];
            double    spread      = 1.0;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              spread *= basisValue(referenceCoordinate(pointSource, axis), nodeIndex(axis));
            }
            const double stateValue = cellState[cellNode * CellLayout::NumberOfData + unknown];
            expected += spread * inverseCellQuadratureWeights[cellNode] / cellVolume
              * pointSourceForce(unknown, pointSource, timePosition, stateValue);
          }
        }
        validateNumericalEqualsWithParams3(
          pointSourceValues[(timeSample * CellLayout::NodesPerCell + cellNode) * ForceStride + unknown],
          expected,
          timeSample,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testTheTimeIntegralSkipsTheSampleAtTheStart() {
  double pointSourceValues[SampleSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < ForceStride; unknown++) {
      pointSourceValues[cellNode * ForceStride + unknown] = 1000.0;
    }
  }
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < ForceStride; unknown++) {
        pointSourceValues[((timeNode + 1) * CellLayout::NodesPerCell + cellNode) * ForceStride + unknown] =
          1.0 + unknown + timeNode + 0.25 * cellNode;
      }
    }
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = seedOfState(entry);
  }

  exahype2::dg::ader::
    addPointSourcesToPredictor<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      state,
      pointSourceValues,
      QuadratureWeights,
      TimeStepSize
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::PaddedNumberOfData; value++) {
      const int entry    = cellNode * CellLayout::PaddedNumberOfData + value;
      double    expected = seedOfState(entry);
      if (value < NumberOfUnknowns) {
        for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
          expected += TimeStepSize * QuadratureWeights[timeNode] * (1.0 + value + timeNode + 0.25 * cellNode);
        }
      }
      validateNumericalEqualsWithParams2(state[entry], expected, cellNode, value);
    }
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testTheDerivativeCoefficientsWeightTheUnknownsOnly() {
  double derivativeCoefficients[NumberOfDofsPerAxis];
  double coefficientSum = 0.0;
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    derivativeCoefficients[timeNode] = 0.5 + timeNode;
    coefficientSum += derivativeCoefficients[timeNode];
  }

  // Every time sample of a node carries the same value, so the sum over the samples is the sum of the
  // coefficients times that value.
  double pointSourceValues[SampleSize];
  for (int timeSample = 0; timeSample < NumberOfDofsPerAxis + 1; timeSample++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < ForceStride; unknown++) {
        pointSourceValues[(timeSample * CellLayout::NodesPerCell + cellNode) * ForceStride + unknown] =
          (unknown < NumberOfUnknowns) ? (1.0 + unknown + 0.25 * cellNode) : 100.0;
      }
    }
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = seedOfState(entry);
  }

  exahype2::dg::ader::
    addPointSourceDerivativeToState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      state,
      pointSourceValues,
      derivativeCoefficients
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::PaddedNumberOfData; value++) {
      const int entry    = cellNode * CellLayout::PaddedNumberOfData + value;
      double    expected = seedOfState(entry);
      if (value < NumberOfUnknowns) {
        expected += coefficientSum * (1.0 + value + 0.25 * cellNode);
      }
      validateNumericalEqualsWithParams2(state[entry], expected, cellNode, value);
    }
  }
}

void exahype2::dg::ader::tests::PointSourceContributionTest::testEveryAxisReceivesTheSameTimeAverage() {
  double pointSourceValues[SampleSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < ForceStride; unknown++) {
      pointSourceValues[cellNode * ForceStride + unknown] = 1000.0;
    }
  }
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < ForceStride; unknown++) {
        pointSourceValues[((timeNode + 1) * CellLayout::NodesPerCell + cellNode) * ForceStride + unknown] =
          1.0 + unknown + timeNode + 0.25 * cellNode;
      }
    }
  }

  double timeAveragedFlux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    timeAveragedFlux[entry] = 0.05 * entry;
  }

  exahype2::dg::ader::
    addPointSourcesToTimeAveragedFlux<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      timeAveragedFlux,
      pointSourceValues,
      QuadratureWeights
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < ForceStride; unknown++) {
      double timeAverage = 0.0;
      for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
        timeAverage += QuadratureWeights[timeNode] / DIMENSIONS * (1.0 + unknown + timeNode + 0.25 * cellNode);
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const int entry = axis * AxisStride + cellNode * ForceStride + unknown;
        validateNumericalEqualsWithParams3(
          timeAveragedFlux[entry],
          0.05 * entry + timeAverage,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}
