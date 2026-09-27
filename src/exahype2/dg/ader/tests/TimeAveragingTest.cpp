// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeAveragingTest.h"

#include "exahype2/dg/ader/TimeAveraging.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateStride = CellLayout::PaddedNumberOfData;

  // Gauss-Legendre weights of three points on the unit interval, which sum to one.
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  double stateValueAt(const int cellNode, const int value) { return 1.0 + 0.25 * cellNode + 0.5 * value; }

  double sampleAt(const int timeNode, const int cellNode, const int value) {
    return stateValueAt(cellNode, value) + 0.25 * timeNode * (1 + value);
  }

  struct SpaceTimeState final {
    double values[CellLayout::SpaceTimeNodes * StateStride]{};
  };

  void fillConstantInTime(SpaceTimeState& state) {
    for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        for (int value = 0; value < StateStride; value++) {
          state.values[spaceTimeNode * StateStride + value] = stateValueAt(cellNode, value);
        }
      }
    }
  }

  void fillVaryingInTime(SpaceTimeState& state) {
    for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        for (int value = 0; value < StateStride; value++) {
          state.values[spaceTimeNode * StateStride + value] = sampleAt(timeNode, cellNode, value);
        }
      }
    }
  }
} // namespace

exahype2::dg::ader::tests::TimeAveragingTest::TimeAveragingTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::TimeAveragingTest") {}

void exahype2::dg::ader::tests::TimeAveragingTest::run() {
  testMethod(testTheAverageOfAConstantIsThatConstant);
  testMethod(testEveryTimeSliceEntersWithItsWeight);
  testMethod(testTheLaterTimeSlicesKeepTheirValues);
}

void exahype2::dg::ader::tests::TimeAveragingTest::testTheAverageOfAConstantIsThatConstant() {
  SpaceTimeState state;
  fillConstantInTime(state);

  exahype2::dg::ader::averageStateOverTime<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
    state.values,
    QuadratureWeights
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(
        state.values[cellNode * StateStride + value],
        stateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testEveryTimeSliceEntersWithItsWeight() {
  SpaceTimeState state;
  fillVaryingInTime(state);

  exahype2::dg::ader::averageStateOverTime<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
    state.values,
    QuadratureWeights
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      double expected = 0.0;
      for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
        expected += QuadratureWeights[timeNode] * sampleAt(timeNode, cellNode, value);
      }
      validateNumericalEqualsWithParams2(state.values[cellNode * StateStride + value], expected, cellNode, value);
    }
  }
}

void exahype2::dg::ader::tests::TimeAveragingTest::testTheLaterTimeSlicesKeepTheirValues() {
  SpaceTimeState state;
  fillVaryingInTime(state);

  exahype2::dg::ader::averageStateOverTime<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
    state.values,
    QuadratureWeights
  );

  for (int timeNode = 1; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < StateStride; value++) {
        validateNumericalEqualsWithParams3(
          state.values[spaceTimeNode * StateStride + value],
          sampleAt(timeNode, cellNode, value),
          timeNode,
          cellNode,
          value
        );
      }
    }
  }
}
