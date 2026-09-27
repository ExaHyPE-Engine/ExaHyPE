// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpaceTimeTermsTest.h"

#include "exahype2/dg/ader/SpaceTimeTerms.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int OperatorEntries            = NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateStride        = CellLayout::PaddedNumberOfData;
  constexpr int UnknownStride      = CellLayout::PaddedNumberOfUnknowns;
  constexpr int FluxAxisStride     = CellLayout::NodesPerCell * UnknownStride;
  constexpr int GradientAxisStride = CellLayout::NodesPerCell * UnknownStride;

  constexpr int    MarkedTimeNode = 1;
  constexpr double TimeStamp      = 0.75;
  constexpr double TimeStepSize   = 0.125;

  // Gauss-Legendre nodes and weights of three points on the unit interval.
  const double QuadratureNodes[NumberOfDofsPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  // Derivative of the Lagrange basis function of the given node at x.
  double basisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < NumberOfDofsPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < NumberOfDofsPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  // Entry (i, j) holds the derivative of basis function j at node i.
  void fillDerivativeOperator(double (&derivativeOperator)[OperatorEntries]) {
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        derivativeOperator[row * NumberOfDofsPerAxis + column] = basisDerivative(QuadratureNodes[row], column);
      }
    }
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

  tarch::la::Vector<DIMENSIONS, double> makeCellCentre() {
    tarch::la::Vector<DIMENSIONS, double> cellCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellCentre(axis) = 1.0 + axis;
    }
    return cellCentre;
  }

  tarch::la::Vector<DIMENSIONS, double> nodePositionAt(const int cellNode) {
    const auto                            cellSize   = makeCellSize();
    const auto                            cellCentre = makeCellCentre();
    const auto                            nodeIndex  = nodeIndexOf(cellNode);
    tarch::la::Vector<DIMENSIONS, double> position;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) = cellCentre(axis) - 0.5 * cellSize(axis) + QuadratureNodes[nodeIndex(axis)] * cellSize(axis);
    }
    return position;
  }

  double timePositionOfMarkedNode() { return TimeStamp + QuadratureNodes[MarkedTimeNode] * TimeStepSize; }

  double cellQuadratureWeightAt(const int cellNode) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    double     weight    = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      weight *= QuadratureWeights[nodeIndex(axis)];
    }
    return weight;
  }

  double stateValueAt(const int cellNode, const int value) { return 1.0 + 0.5 * cellNode + value; }

  double gradientValueAt(const int axis, const int cellNode, const int unknown) {
    return 0.25 + axis + 0.5 * unknown + 0.125 * cellNode;
  }

  void fillSpaceTimeState(double (&spaceTimeState)[CellLayout::SpaceTimeNodes * StateStride]) {
    for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
      spaceTimeState[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        spaceTimeState[spaceTimeNode * StateStride + value] = stateValueAt(cellNode, value);
      }
    }
  }

  void fillStateGradient(double (&stateGradient)[DIMENSIONS * GradientAxisStride]) {
    for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
      stateGradient[entry] = 0.0;
    }
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown] =
            gradientValueAt(axis, cellNode, unknown);
        }
      }
    }
  }

  constexpr double InitialTimeAverage = 0.75;

  void resetSourceBuffers(
    double (&timeAveragedSource)[CellLayout::NodesPerCell * UnknownStride],
    double (&rightHandSide)[CellLayout::SpaceTimeNodes * UnknownStride]
  ) {
    for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
      timeAveragedSource[entry] = InitialTimeAverage;
    }
    for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
      rightHandSide[entry] = 0.5;
    }
  }

  struct MarkerSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown] + position(direction) + timeStamp;
      }
    }

    static void flux(
      const double* NOALIAS const                                   state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown];
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (0.5 + axis) * stateGradients[axis][unknown];
        }
      }
    }

    static void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = 2.0 * state[unknown] + timeStamp;
      }
    }

    static void diffusiveSourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          sourceValues[unknown] += (1.0 + axis) * stateGradient[axis * NumberOfUnknowns + unknown];
        }
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] = (1.0 + 0.25 * direction) * stateGradient[unknown];
      }
    }
  };
} // namespace

exahype2::dg::ader::tests::SpaceTimeTermsTest::SpaceTimeTermsTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::SpaceTimeTermsTest") {}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::run() {
  testMethod(testTheNodeIndexSplitsTheFlattenedIndex);
  testMethod(testTheGradientOfAConstantVanishes);
  testMethod(testALinearStateHasTheExactGradient);
  testMethod(testTheFluxIsAskedForOncePerNodeAndAxis);
  testMethod(testTheDiffusiveFluxSeesTheNodalGradients);
  testMethod(testEveryTermCombinationAccumulatesIntoOneBuffer);
  testMethod(testTheFirstTimeNodeRestartsTheTimeAverages);
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testTheNodeIndexSplitsTheFlattenedIndex() {
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const auto nodeIndex = exahype2::dg::ader::spatialNodeIndex<NumberOfDofsPerAxis>(cellNode);

    int rebuilt = 0;
    int stride  = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateWithParams2(nodeIndex(axis) >= 0, cellNode, axis);
      validateWithParams2(nodeIndex(axis) < NumberOfDofsPerAxis, cellNode, axis);
      rebuilt += nodeIndex(axis) * stride;
      stride *= NumberOfDofsPerAxis;
    }
    validateEqualsWithParams1(rebuilt, cellNode, cellNode);
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testTheGradientOfAConstantVanishes() {
  double derivativeOperator[OperatorEntries];
  fillDerivativeOperator(derivativeOperator);

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
    spaceTimeState[entry] = 3.0;
  }

  double stateGradient[DIMENSIONS * GradientAxisStride];
  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    stateGradient[entry] = 9.0;
  }

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 + axis;
  }

  exahype2::dg::ader::
    computeSpaceTimeGradient<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      stateGradient,
      spaceTimeState,
      derivativeOperator,
      inverseCellSize,
      MarkedTimeNode
    );

  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    validateNumericalEqualsWithParams1(stateGradient[entry], 0.0, entry);
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testALinearStateHasTheExactGradient() {
  constexpr double Slope = 3.0;

  double derivativeOperator[OperatorEntries];
  fillDerivativeOperator(derivativeOperator);

  // The state grows linearly along every axis, so the nodal derivative of the interpolant is the slope.
  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride] = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const auto nodeIndex     = nodeIndexOf(cellNode);
    const int  spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    double     reference     = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      reference += (1.0 + axis) * Slope * QuadratureNodes[nodeIndex(axis)];
    }
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      spaceTimeState[spaceTimeNode * StateStride + value] = (1.0 + value) * (2.0 + reference);
    }
  }

  double stateGradient[DIMENSIONS * GradientAxisStride];
  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    stateGradient[entry] = 9.0;
  }

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
  }

  exahype2::dg::ader::
    computeSpaceTimeGradient<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      stateGradient,
      spaceTimeState,
      derivativeOperator,
      inverseCellSize,
      MarkedTimeNode
    );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expected = (1.0 + unknown) * (1.0 + axis) * Slope * inverseCellSize(axis);
        validateNumericalEqualsWithParams3(
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown],
          0.0,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testTheFluxIsAskedForOncePerNodeAndAxis() {
  const auto cellCentre = makeCellCentre();
  const auto cellSize   = makeCellSize();

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double flux[DIMENSIONS * FluxAxisStride];
  double timeAveragedFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    flux[entry]             = 9.0;
    timeAveragedFlux[entry] = InitialTimeAverage;
  }

  exahype2::dg::ader::evaluateSpaceTimeFlux<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    double>(
    flux,
    timeAveragedFlux,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );

  const double timePosition = timePositionOfMarkedNode();

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double position = nodePositionAt(cellNode)(axis);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const int    entry    = axis * FluxAxisStride + cellNode * UnknownStride + unknown;
        const double expected = (1.0 + axis) * stateValueAt(cellNode, unknown) + position + timePosition;
        validateNumericalEqualsWithParams3(flux[entry], expected, axis, cellNode, unknown);
        validateNumericalEqualsWithParams3(
          timeAveragedFlux[entry],
          InitialTimeAverage + QuadratureWeights[MarkedTimeNode] * expected,
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        const int entry = axis * FluxAxisStride + cellNode * UnknownStride + unknown;
        validateNumericalEqualsWithParams3(flux[entry], 0.0, axis, cellNode, unknown);
        validateNumericalEqualsWithParams3(timeAveragedFlux[entry], InitialTimeAverage, axis, cellNode, unknown);
      }
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testTheDiffusiveFluxSeesTheNodalGradients() {
  const auto cellCentre = makeCellCentre();
  const auto cellSize   = makeCellSize();

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double flux[DIMENSIONS * FluxAxisStride];
  double timeAveragedFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    flux[entry]             = 9.0;
    timeAveragedFlux[entry] = InitialTimeAverage;
  }

  exahype2::dg::ader::
    evaluateSpaceTimeFlux<MarkerSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, double>(
      flux,
      timeAveragedFlux,
      spaceTimeState,
      stateGradient,
      QuadratureNodes,
      QuadratureWeights,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize,
      MarkedTimeNode
    );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = (1.0 + axis) * stateValueAt(cellNode, unknown);
        for (int gradientAxis = 0; gradientAxis < DIMENSIONS; gradientAxis++) {
          expected += (0.5 + gradientAxis) * gradientValueAt(gradientAxis, cellNode, unknown);
        }
        validateNumericalEqualsWithParams3(
          flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testEveryTermCombinationAccumulatesIntoOneBuffer() {
  const auto cellCentre = makeCellCentre();
  const auto cellSize   = makeCellSize();

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double cellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    cellQuadratureWeights[cellNode] = cellQuadratureWeightAt(cellNode);
  }

  double timeAveragedSource[CellLayout::NodesPerCell * UnknownStride];
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    false,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, false, true, false);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, true, false, false);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    true,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, false, false, true);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    false,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, true, true, false);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    true,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, true, false, true);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    true,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, false, true, true);

  resetSourceBuffers(timeAveragedSource, rightHandSide);
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );
  validateSourceCombination(rightHandSide, timeAveragedSource, true, true, true);
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::testTheFirstTimeNodeRestartsTheTimeAverages() {
  const auto cellCentre = makeCellCentre();
  const auto cellSize   = makeCellSize();

  // The first time slice holds zeros, so the callbacks depend on the node position and time only.
  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double cellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    cellQuadratureWeights[cellNode] = cellQuadratureWeightAt(cellNode);
  }

  double flux[DIMENSIONS * FluxAxisStride];
  double timeAveragedFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    timeAveragedFlux[entry] = 9.0;
  }
  double timeAveragedSource[CellLayout::NodesPerCell * UnknownStride];
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  resetSourceBuffers(timeAveragedSource, rightHandSide);

  exahype2::dg::ader::evaluateSpaceTimeFlux<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    double>(
    flux,
    timeAveragedFlux,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    0
  );
  exahype2::dg::ader::evaluateSpaceTimeSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    false,
    double>(
    rightHandSide,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    QuadratureNodes,
    QuadratureWeights,
    cellQuadratureWeights,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    0
  );

  const double timePosition = TimeStamp + QuadratureNodes[0] * TimeStepSize;
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      const double source = (unknown < NumberOfUnknowns) ? timePosition : 0.0;
      validateNumericalEqualsWithParams2(
        timeAveragedSource[cellNode * UnknownStride + unknown],
        QuadratureWeights[0] * source,
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double nodeFlux = (unknown < NumberOfUnknowns) ? nodePositionAt(cellNode)(axis) + timePosition : 0.0;
        validateNumericalEqualsWithParams3(
          timeAveragedFlux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          QuadratureWeights[0] * nodeFlux,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimeTermsTest::validateSourceCombination(
  const double* const rightHandSide,
  const double* const timeAveragedSource,
  const bool          evaluateNonconservativeProduct,
  const bool          evaluateSource,
  const bool          evaluateDiffusiveSource
) {
  const double timePosition = timePositionOfMarkedNode();

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    const double updateSize    = QuadratureWeights[MarkedTimeNode] * cellQuadratureWeightAt(cellNode) * TimeStepSize;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      if (evaluateSource) {
        expected += 2.0 * stateValueAt(cellNode, unknown) + timePosition;
      }
      if (evaluateDiffusiveSource) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          expected += (1.0 + axis) * gradientValueAt(axis, cellNode, unknown);
        }
      }
      if (evaluateNonconservativeProduct) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          expected -= (1.0 + 0.25 * axis) * gradientValueAt(axis, cellNode, unknown);
        }
      }
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5 + updateSize * expected,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        timeAveragedSource[cellNode * UnknownStride + unknown],
        InitialTimeAverage + QuadratureWeights[MarkedTimeNode] * expected,
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        timeAveragedSource[cellNode * UnknownStride + unknown],
        InitialTimeAverage,
        cellNode,
        unknown
      );
    }
  }

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    if (timeNode != MarkedTimeNode) {
      const int firstEntry = timeNode * CellLayout::NodesPerCell * UnknownStride;
      for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
        validateNumericalEqualsWithParams2(rightHandSide[firstEntry + entry], 0.5, timeNode, entry);
      }
    }
  }
}
