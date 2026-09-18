// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LinearCombinationTest.h"

#include "exahype2/dg/rk/LinearCombination.h"

namespace {
  constexpr int NumberOfRungeKuttaSteps    = 2;
  constexpr int NumberOfDofsPerAxis        = 2;
  constexpr int NumberOfUnknowns           = 1;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int NodesPerCell = (DIMENSIONS == 3) ? NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis
                                                 : NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  constexpr double ButcherTableau[NumberOfRungeKuttaSteps * NumberOfRungeKuttaSteps] = {0.25, -0.5, 0.5, 0.25};

  void fillStateAndEstimates(double* NOALIAS const cellState, double* NOALIAS const rightHandSideEstimates) {
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      const int stateEntry                     = cellNode * NumberOfData;
      cellState[stateEntry]                    = 10.0 + cellNode;
      cellState[stateEntry + NumberOfUnknowns] = 100.0 + cellNode;
      for (int estimate = 0; estimate < NumberOfRungeKuttaSteps; estimate++) {
        rightHandSideEstimates[estimate * NodesPerCell + cellNode] = 1.0 + 2.0 * estimate + cellNode;
      }
    }
  }
} // namespace

exahype2::dg::rk::tests::LinearCombinationTest::LinearCombinationTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::LinearCombinationTest") {}

void exahype2::dg::rk::tests::LinearCombinationTest::run() {
  testMethod(testEachStepReadsItsOwnRowOfTheTableau);
  testMethod(testTheFinalStepCombinesInPlace);
  testMethod(testZeroWeightsSkipUncomputedEstimates);
}

/** Checks each stage's state against its tableau row. */
void exahype2::dg::rk::tests::LinearCombinationTest::testEachStepReadsItsOwnRowOfTheTableau() {
  double cellState[NodesPerCell * NumberOfData];
  double rightHandSideEstimates[NumberOfRungeKuttaSteps * NodesPerCell * NumberOfUnknowns];
  fillStateAndEstimates(cellState, rightHandSideEstimates);

  for (int rungeKuttaStep = 0; rungeKuttaStep < NumberOfRungeKuttaSteps; rungeKuttaStep++) {
    double stageState[NodesPerCell * NumberOfData] = {};
    exahype2::dg::rk::linearCombinationOfEstimates<
      NumberOfRungeKuttaSteps,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables>(rungeKuttaStep, cellState, rightHandSideEstimates, stageState, ButcherTableau);

    const int           rowOffset = rungeKuttaStep * NumberOfRungeKuttaSteps;
    const double* const row       = ButcherTableau + rowOffset;
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      const int stateEntry = cellNode * NumberOfData;
      double    expected   = cellState[stateEntry];
      for (int estimate = 0; estimate < NumberOfRungeKuttaSteps; estimate++) {
        expected += row[estimate] * rightHandSideEstimates[estimate * NodesPerCell + cellNode];
      }
      validateNumericalEqualsWithParams2(stageState[stateEntry], expected, rungeKuttaStep, cellNode);
      validateNumericalEqualsWithParams2(
        stageState[stateEntry + NumberOfUnknowns],
        cellState[stateEntry + NumberOfUnknowns],
        rungeKuttaStep,
        cellNode
      );
    }
  }
}

/** Checks that the final combination can overwrite its input state. */
void exahype2::dg::rk::tests::LinearCombinationTest::testTheFinalStepCombinesInPlace() {
  double cellState[NodesPerCell * NumberOfData];
  double rightHandSideEstimates[NumberOfRungeKuttaSteps * NodesPerCell * NumberOfUnknowns];
  fillStateAndEstimates(cellState, rightHandSideEstimates);

  double stageState[NodesPerCell * NumberOfData] = {};
  exahype2::dg::rk::linearCombinationOfEstimates<
    NumberOfRungeKuttaSteps,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(0, cellState, rightHandSideEstimates, stageState, ButcherTableau);

  double finalWeights[NumberOfRungeKuttaSteps];
  for (int estimate = 0; estimate < NumberOfRungeKuttaSteps; estimate++) {
    finalWeights[estimate] = ButcherTableau[estimate];
  }
  double finalState[NodesPerCell * NumberOfData];
  std::copy_n(cellState, NodesPerCell * NumberOfData, finalState);
  exahype2::dg::rk::linearCombinationOfEstimates<
    NumberOfRungeKuttaSteps,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(NumberOfRungeKuttaSteps, finalState, rightHandSideEstimates, finalState, finalWeights);

  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    validateNumericalEqualsWithParams1(finalState[entry], stageState[entry], entry);
  }
}

/** Checks that zero weights exclude uncomputed estimates from the state. */
void exahype2::dg::rk::tests::LinearCombinationTest::testZeroWeightsSkipUncomputedEstimates() {
  double cellState[NodesPerCell * NumberOfData];
  double rightHandSideEstimates[NumberOfRungeKuttaSteps * NodesPerCell * NumberOfUnknowns];
  fillStateAndEstimates(cellState, rightHandSideEstimates);
  std::fill_n(rightHandSideEstimates, NodesPerCell * NumberOfUnknowns, std::numeric_limits<double>::quiet_NaN());
  constexpr double FinalWeights[NumberOfRungeKuttaSteps] = {0.0, 0.5};
  double           stageState[NodesPerCell * NumberOfData]{};
  exahype2::dg::rk::linearCombinationOfEstimates<
    NumberOfRungeKuttaSteps,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(NumberOfRungeKuttaSteps, cellState, rightHandSideEstimates, stageState, FinalWeights);
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    const int    stateEntry = cellNode * NumberOfData;
    const double expected   = cellState[stateEntry] + 0.5 * rightHandSideEstimates[NodesPerCell + cellNode];
    validateNumericalEqualsWithParams1(stageState[stateEntry], expected, cellNode);
    validateNumericalEqualsWithParams1(
      stageState[stateEntry + NumberOfUnknowns],
      cellState[stateEntry + NumberOfUnknowns],
      cellNode
    );
  }
}
