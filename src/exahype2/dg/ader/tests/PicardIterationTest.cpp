// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PicardIterationTest.h"

#include "exahype2/dg/ader/PicardIteration.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int OperatorEntries            = NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateStride    = CellLayout::PaddedNumberOfData;
  constexpr int UnknownStride  = CellLayout::PaddedNumberOfUnknowns;
  constexpr int FluxAxisStride = CellLayout::SpaceTimeNodes * UnknownStride;

  // Gauss-Legendre nodes and weights of three points on the unit interval.
  const double QuadratureNodes[NumberOfDofsPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  // Lagrange basis function of the given node evaluated at x.
  double basisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NumberOfDofsPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

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

  tarch::la::Vector<DIMENSIONS, int> nodeIndexOf(const int cellNode) {
    tarch::la::Vector<DIMENSIONS, int> nodeIndex;
    int                                remainder = cellNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
      remainder /= NumberOfDofsPerAxis;
    }
    return nodeIndex;
  }

  void fillIdentity(double (&matrix)[OperatorEntries]) {
    for (int entry = 0; entry < OperatorEntries; entry++) {
      matrix[entry] = 0.0;
    }
    for (int index = 0; index < NumberOfDofsPerAxis; index++) {
      matrix[index * NumberOfDofsPerAxis + index] = 1.0;
    }
  }

  // Gauss-Jordan elimination with partial pivoting.
  void invertOperator(const double (&matrix)[OperatorEntries], double (&inverse)[OperatorEntries]) {
    double work[OperatorEntries];
    for (int entry = 0; entry < OperatorEntries; entry++) {
      work[entry]    = matrix[entry];
      inverse[entry] = 0.0;
    }
    for (int index = 0; index < NumberOfDofsPerAxis; index++) {
      inverse[index * NumberOfDofsPerAxis + index] = 1.0;
    }
    for (int column = 0; column < NumberOfDofsPerAxis; column++) {
      int pivotRow = column;
      for (int row = column + 1; row < NumberOfDofsPerAxis; row++) {
        if (std::fabs(work[row * NumberOfDofsPerAxis + column])
            > std::fabs(work[pivotRow * NumberOfDofsPerAxis + column])) {
          pivotRow = row;
        }
      }
      for (int entry = 0; entry < NumberOfDofsPerAxis; entry++) {
        std::swap(work[column * NumberOfDofsPerAxis + entry], work[pivotRow * NumberOfDofsPerAxis + entry]);
        std::swap(inverse[column * NumberOfDofsPerAxis + entry], inverse[pivotRow * NumberOfDofsPerAxis + entry]);
      }
      const double scaling = 1.0 / work[column * NumberOfDofsPerAxis + column];
      for (int entry = 0; entry < NumberOfDofsPerAxis; entry++) {
        work[column * NumberOfDofsPerAxis + entry] *= scaling;
        inverse[column * NumberOfDofsPerAxis + entry] *= scaling;
      }
      for (int row = 0; row < NumberOfDofsPerAxis; row++) {
        if (row != column) {
          const double factor = work[row * NumberOfDofsPerAxis + column];
          for (int entry = 0; entry < NumberOfDofsPerAxis; entry++) {
            work[row * NumberOfDofsPerAxis + entry] -= factor * work[column * NumberOfDofsPerAxis + entry];
            inverse[row * NumberOfDofsPerAxis + entry] -= factor * inverse[column * NumberOfDofsPerAxis + entry];
          }
        }
      }
    }
  }

  struct Tables final {
    double cellQuadratureWeights[CellLayout::NodesPerCell]{};
    double inverseCellQuadratureWeights[CellLayout::NodesPerCell]{};
    double stiffnessOperatorTransposed[OperatorEntries]{};
    double inversePicardOperator[OperatorEntries]{};
    double leftFaceCoefficients[NumberOfDofsPerAxis]{};
    double rightFaceCoefficients[NumberOfDofsPerAxis]{};
  };

  // The Picard operator of the temporal system is the endpoint term of the integration by parts
  // minus the stiffness operator.
  Tables makeTables() {
    Tables tables;
    for (int node = 0; node < NumberOfDofsPerAxis; node++) {
      tables.leftFaceCoefficients[node]  = basisValue(0.0, node);
      tables.rightFaceCoefficients[node] = basisValue(1.0, node);
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = nodeIndexOf(cellNode);
      double     weight    = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        weight *= QuadratureWeights[nodeIndex(axis)];
      }
      tables.cellQuadratureWeights[cellNode]        = weight;
      tables.inverseCellQuadratureWeights[cellNode] = 1.0 / weight;
    }
    double picardOperator[OperatorEntries];
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        const double stiffness = QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row);
        tables.stiffnessOperatorTransposed[column * NumberOfDofsPerAxis + row] = stiffness;
        picardOperator[row * NumberOfDofsPerAxis + column] =
          tables.rightFaceCoefficients[row] * tables.rightFaceCoefficients[column] - stiffness;
      }
    }
    invertOperator(picardOperator, tables.inversePicardOperator);
    return tables;
  }
} // namespace

exahype2::dg::ader::tests::PicardIterationTest::PicardIterationTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PicardIterationTest") {}

void exahype2::dg::ader::tests::PicardIterationTest::run() {
  testMethod(testTheGuessRepeatsTheCellStateOverTime);
  testMethod(testTheRightHandSideCarriesTheInitialValue);
  testMethod(testAConstantFluxHasNoDivergence);
  testMethod(testALinearFluxSubtractsItsExactDivergence);
  testMethod(testAPicardSystemWithoutTermsReproducesTheInitialState);
  testMethod(testTheResidualIsTheSquaredDistanceOfTheIterates);
  testMethod(testAnUnboundedIterateReportsDivergence);
}

void exahype2::dg::ader::tests::PicardIterationTest::testTheGuessRepeatsTheCellStateOverTime() {
  float cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int entry = 0; entry < CellLayout::NodesPerCell * CellLayout::NumberOfData; entry++) {
    cellState[entry] = 1.0F + static_cast<float>(entry);
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
    spaceTimeState[entry] = 9.0;
  }

  exahype2::dg::ader::
    initialisePredictorState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, float, double>(
      spaceTimeState,
      cellState
    );

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        validateNumericalEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + value],
          static_cast<double>(cellState[cellNode * CellLayout::NumberOfData + value]),
          timeNode,
          cellNode,
          value
        );
      }
      for (int value = CellLayout::NumberOfData; value < StateStride; value++) {
        validateNumericalEqualsWithParams2(spaceTimeState[spaceTimeNode * StateStride + value], 0.0, cellNode, value);
      }
    }
  }
}

void exahype2::dg::ader::tests::PicardIterationTest::testTheRightHandSideCarriesTheInitialValue() {
  const Tables tables = makeTables();

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int entry = 0; entry < CellLayout::NodesPerCell * CellLayout::NumberOfData; entry++) {
    cellState[entry] = 1.0 + entry;
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 9.0;
  }

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    exahype2::dg::ader::
      initialisePicardRightHandSide<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
        rightHandSide,
        cellState,
        tables.cellQuadratureWeights,
        tables.leftFaceCoefficients,
        timeNode
      );
  }

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int    spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      const double weight        = tables.cellQuadratureWeights[cellNode] * tables.leftFaceCoefficients[timeNode];
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          rightHandSide[spaceTimeNode * UnknownStride + unknown],
          weight * cellState[cellNode * CellLayout::NumberOfData + unknown],
          timeNode,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams2(
          rightHandSide[spaceTimeNode * UnknownStride + unknown],
          0.0,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::PicardIterationTest::testAConstantFluxHasNoDivergence() {
  const Tables tables = makeTables();

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
  }

  double spaceTimeFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    spaceTimeFlux[entry] = 5.0;
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 7.0;
  }

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    exahype2::dg::ader::
      subtractFluxDivergence<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
        rightHandSide,
        spaceTimeFlux,
        tables.stiffnessOperatorTransposed,
        tables.cellQuadratureWeights,
        inverseCellSize,
        0.25,
        timeNode
      );
  }

  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    validateNumericalEqualsWithParams1(rightHandSide[entry], 7.0, entry);
  }
}

void exahype2::dg::ader::tests::PicardIterationTest::testALinearFluxSubtractsItsExactDivergence() {
  constexpr int    MarkedTimeNode = 1;
  constexpr double TimeStepSize   = 0.25;
  constexpr double Slope          = 3.0;

  const Tables tables = makeTables();

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
  }

  // The flux block of an axis grows linearly along that axis alone, so the nodal derivative of the
  // interpolant is the slope itself.
  double spaceTimeFlux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int    spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        const double reference     = QuadratureNodes[nodeIndexOf(cellNode)(axis)];
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          spaceTimeFlux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown] =
            (1.0 + unknown) * (2.0 + Slope * (1.0 + axis) * reference);
        }
      }
    }
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};

  exahype2::dg::ader::subtractFluxDivergence<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
    rightHandSide,
    spaceTimeFlux,
    tables.stiffnessOperatorTransposed,
    tables.cellQuadratureWeights,
    inverseCellSize,
    TimeStepSize,
    MarkedTimeNode
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    const double weight        = QuadratureWeights[MarkedTimeNode] * tables.cellQuadratureWeights[cellNode];
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        expected -= weight * TimeStepSize * inverseCellSize(axis) * (1.0 + unknown) * Slope * (1.0 + axis);
      }
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        expected,
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.0,
        cellNode,
        unknown
      );
    }
  }

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    if (timeNode != MarkedTimeNode) {
      for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
        validateNumericalEqualsWithParams2(
          rightHandSide[timeNode * CellLayout::NodesPerCell * UnknownStride + entry],
          0.0,
          timeNode,
          entry
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::PicardIterationTest::testAPicardSystemWithoutTermsReproducesTheInitialState() {
  const Tables tables = makeTables();

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      cellState[cellNode * CellLayout::NumberOfData + value] = 1.0 + 0.5 * cellNode + value;
    }
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  exahype2::dg::ader::
    initialisePredictorState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      spaceTimeState,
      cellState
    );

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    exahype2::dg::ader::
      initialisePicardRightHandSide<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
        rightHandSide,
        cellState,
        tables.cellQuadratureWeights,
        tables.leftFaceCoefficients,
        timeNode
      );
  }

  double updatedTimeSlice[NumberOfDofsPerAxis * UnknownStride] = {};

  const double squaredResidual =
    exahype2::dg::ader::solvePicardSystem<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      spaceTimeState,
      updatedTimeSlice,
      rightHandSide,
      tables.inversePicardOperator,
      tables.inverseCellQuadratureWeights
    );

  validateNumericalEquals(squaredResidual, 0.0);

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        validateNumericalEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + value],
          cellState[cellNode * CellLayout::NumberOfData + value],
          timeNode,
          cellNode,
          value
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::PicardIterationTest::testTheResidualIsTheSquaredDistanceOfTheIterates() {
  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double updatedTimeSlice[NumberOfDofsPerAxis * UnknownStride]     = {};

  double expectedResidual = 0.0;
  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double newValue = 1.0 + 0.5 * (cellNode + unknown + timeNode);
        rightHandSide[(timeNode * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] = newValue;
        expectedResidual += newValue * newValue;
      }
    }
  }

  const double squaredResidual =
    exahype2::dg::ader::solvePicardSystem<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      spaceTimeState,
      updatedTimeSlice,
      rightHandSide,
      inversePicardOperator,
      inverseCellQuadratureWeights
    );

  validateNumericalEquals(squaredResidual, expectedResidual);

  for (int timeNode = 0; timeNode < NumberOfDofsPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + unknown],
          1.0 + 0.5 * (cellNode + unknown + timeNode),
          timeNode,
          cellNode,
          unknown
        );
      }
    }
  }

  const double secondResidual =
    exahype2::dg::ader::solvePicardSystem<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      spaceTimeState,
      updatedTimeSlice,
      rightHandSide,
      inversePicardOperator,
      inverseCellQuadratureWeights
    );

  validateNumericalEquals(secondResidual, 0.0);
}

void exahype2::dg::ader::tests::PicardIterationTest::testAnUnboundedIterateReportsDivergence() {
  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double updatedTimeSlice[NumberOfDofsPerAxis * UnknownStride]     = {};

  rightHandSide[0] = 1e30;
  const double unboundedResidual =
    exahype2::dg::ader::solvePicardSystem<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      spaceTimeState,
      updatedTimeSlice,
      rightHandSide,
      inversePicardOperator,
      inverseCellQuadratureWeights
    );
  validate(not std::isfinite(unboundedResidual));

  rightHandSide[0] = std::numeric_limits<double>::quiet_NaN();
  const double notANumberResidual =
    exahype2::dg::ader::solvePicardSystem<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      spaceTimeState,
      updatedTimeSlice,
      rightHandSide,
      inversePicardOperator,
      inverseCellQuadratureWeights
    );
  validate(not std::isfinite(notANumberResidual));
}
