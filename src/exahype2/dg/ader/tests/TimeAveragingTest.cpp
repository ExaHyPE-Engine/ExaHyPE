// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeAveragingTest.h"

#include "exahype2/dg/ader/TimeAveraging.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateStride         = CellLayout::PaddedNumberOfData;
  constexpr int UnknownStride       = CellLayout::PaddedNumberOfUnknowns;
  constexpr int SpaceTimeAxisStride = CellLayout::SpaceTimeNodes * UnknownStride;
  constexpr int AveragedAxisStride  = CellLayout::NodesPerCell * UnknownStride;

  // Gauss-Legendre weights of three points on the unit interval, which sum to one.
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  double stateValueAt(const int cellNode, const int value) { return 1.0 + 0.25 * cellNode + 0.5 * value; }

  double fluxValueAt(const int axis, const int cellNode, const int unknown) {
    return 2.0 + 0.25 * cellNode + axis + 0.5 * unknown;
  }

  double sourceValueAt(const int cellNode, const int unknown) { return 3.0 - 0.25 * cellNode + 0.5 * unknown; }

  struct SpaceTimeBuffers final {
    double state[CellLayout::SpaceTimeNodes * StateStride]{};
    double flux[DIMENSIONS * SpaceTimeAxisStride]{};
    double source[CellLayout::SpaceTimeNodes * UnknownStride]{};
  };

  struct AveragedBuffers final {
    double state[CellLayout::NodesPerCell * StateStride]{};
    double flux[DIMENSIONS * AveragedAxisStride]{};
    double source[AveragedAxisStride]{};
  };

  void fillConstantInTime(SpaceTimeBuffers& buffers) {
    for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        for (int value = 0; value < StateStride; value++) {
          buffers.state[spaceTimeNode * StateStride + value] = stateValueAt(cellNode, value);
        }
        for (int unknown = 0; unknown < UnknownStride; unknown++) {
          buffers.source[spaceTimeNode * UnknownStride + unknown] = sourceValueAt(cellNode, unknown);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            buffers.flux[axis * SpaceTimeAxisStride + spaceTimeNode * UnknownStride + unknown] =
              fluxValueAt(axis, cellNode, unknown);
          }
        }
      }
    }
  }

  void fillTargets(AveragedBuffers& buffers, const double value) {
    for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
      buffers.state[entry] = value;
    }
    for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
      buffers.flux[entry] = value;
    }
    for (int entry = 0; entry < AveragedAxisStride; entry++) {
      buffers.source[entry] = value;
    }
  }
} // namespace

exahype2::dg::ader::tests::TimeAveragingTest::TimeAveragingTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::TimeAveragingTest") {}

void exahype2::dg::ader::tests::TimeAveragingTest::run() {
  testMethod(testTheAverageOfAConstantIsThatConstant);
  testMethod(testTheTargetIsOverwrittenNotAccumulated);
  testMethod(testEveryAxisBlockKeepsCellNodeOrder);
  testMethod(testSwitchedOffTermsAreLeftAlone);
  testMethod(testAPicardPrecisionBelowTheComputePrecisionIsAveraged);
}

void exahype2::dg::ader::tests::TimeAveragingTest::testTheAverageOfAConstantIsThatConstant() {
  SpaceTimeBuffers spaceTime;
  fillConstantInTime(spaceTime);

  AveragedBuffers averaged;
  fillTargets(averaged, 9.0);

  exahype2::dg::ader::averagePredictorOverTime<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    double,
    double>(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    QuadratureWeights
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(
        averaged.state[cellNode * StateStride + value],
        stateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        averaged.source[cellNode * UnknownStride + unknown],
        sourceValueAt(cellNode, unknown),
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams3(
          averaged.flux[axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
          fluxValueAt(axis, cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testTheTargetIsOverwrittenNotAccumulated() {
  const SpaceTimeBuffers spaceTime;

  AveragedBuffers averaged;
  fillTargets(averaged, 9.0);

  exahype2::dg::ader::averagePredictorOverTime<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    double,
    double>(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    QuadratureWeights
  );

  for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.state[entry], 0.0, entry);
  }
  for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.flux[entry], 0.0, entry);
  }
  for (int entry = 0; entry < AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.source[entry], 0.0, entry);
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testEveryAxisBlockKeepsCellNodeOrder() {
  double weightOfFirstNodeOnly[NumberOfDofsPerAxis] = {};
  weightOfFirstNodeOnly[0]                          = 1.0;

  SpaceTimeBuffers spaceTime;
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        spaceTime.flux[axis * SpaceTimeAxisStride + cellNode * UnknownStride + unknown] =
          fluxValueAt(axis, cellNode, unknown);
      }
    }
  }

  AveragedBuffers averaged;
  exahype2::dg::ader::averagePredictorOverTime<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    double,
    double>(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    weightOfFirstNodeOnly
  );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          averaged.flux[axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
          fluxValueAt(axis, cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testSwitchedOffTermsAreLeftAlone() {
  SpaceTimeBuffers spaceTime;
  fillConstantInTime(spaceTime);

  AveragedBuffers averaged;
  fillTargets(averaged, 5.0);

  exahype2::dg::ader::averagePredictorOverTime<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    double,
    double>(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    QuadratureWeights
  );

  for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.flux[entry], 5.0, entry);
  }
  for (int entry = 0; entry < AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.source[entry], 5.0, entry);
  }
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(
        averaged.state[cellNode * StateStride + value],
        stateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testAPicardPrecisionBelowTheComputePrecisionIsAveraged() {
  // Every sample is a multiple of a quarter, so the narrow storage holds it without a rounding error.
  float spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  float spaceTimeFlux[DIMENSIONS * SpaceTimeAxisStride];
  float spaceTimeSource[CellLayout::SpaceTimeNodes * UnknownStride];

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < StateStride; value++) {
        spaceTimeState[spaceTimeNode * StateStride + value] =
          static_cast<float>(stateValueAt(cellNode, value) + 0.25 * timeNode);
      }
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        spaceTimeSource[spaceTimeNode * UnknownStride + unknown] =
          static_cast<float>(sourceValueAt(cellNode, unknown) + 0.25 * timeNode);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          spaceTimeFlux[axis * SpaceTimeAxisStride + spaceTimeNode * UnknownStride + unknown] =
            static_cast<float>(fluxValueAt(axis, cellNode, unknown) + 0.25 * timeNode);
        }
      }
    }
  }

  AveragedBuffers averaged;
  fillTargets(averaged, 9.0);

  exahype2::dg::ader::averagePredictorOverTime<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    double,
    float>(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTimeState,
    spaceTimeFlux,
    spaceTimeSource,
    QuadratureWeights
  );

  double timeShift = 0.0;
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    timeShift += QuadratureWeights[timeNode] * 0.25 * timeNode;
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(
        averaged.state[cellNode * StateStride + value],
        stateValueAt(cellNode, value) + timeShift,
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        averaged.source[cellNode * UnknownStride + unknown],
        sourceValueAt(cellNode, unknown) + timeShift,
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams3(
          averaged.flux[axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
          fluxValueAt(axis, cellNode, unknown) + timeShift,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}
