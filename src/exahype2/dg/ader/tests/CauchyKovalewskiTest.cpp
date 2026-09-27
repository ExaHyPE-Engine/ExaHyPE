// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CauchyKovalewskiTest.h"

#include "exahype2/dg/ader/CauchyKovalewski.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr double TimeStamp                  = 0.5;
  constexpr double TimeStepSize               = 0.25;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int    AxisStride  = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
  constexpr int    StateSize   = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfData;
  constexpr double MarkerValue = 7.0;

  // Gauss-Legendre nodes of three points on the unit interval.
  const double QuadratureNodes[NumberOfDofsPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};

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

  void makeDerivativeOperator(double (&derivativeOperator)[NumberOfDofsPerAxis * NumberOfDofsPerAxis]) {
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        derivativeOperator[row * NumberOfDofsPerAxis + column] = basisDerivative(QuadratureNodes[row], column);
      }
    }
  }

  void makeUnitOperator(double (&unitOperator)[NumberOfDofsPerAxis * NumberOfDofsPerAxis]) {
    for (int entry = 0; entry < NumberOfDofsPerAxis * NumberOfDofsPerAxis; entry++) {
      unitOperator[entry] = 0.0;
    }
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      unitOperator[row * NumberOfDofsPerAxis + row] = 1.0;
    }
  }

  tarch::la::Vector<DIMENSIONS, double> makeInverseCellSize() {
    tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
    }
    return inverseCellSize;
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

  double slopeOfAxis(const int axis) { return 2.0 + axis; }

  struct MarkerSolver final {
    static void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = 4.0;
      }
    }
  };
} // namespace

exahype2::dg::ader::tests::CauchyKovalewskiTest::CauchyKovalewskiTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::CauchyKovalewskiTest") {}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::run() {
  testMethod(testTheSeedCopiesTheCellStateAndClearsThePadding);
  testMethod(testTheTimeAverageStartsFromTheState);
  testMethod(testTheFluxesAreClearedBeforeATaylorStep);
  testMethod(testALinearFluxDifferentiatesToItsPhysicalSlope);
  testMethod(testTheUnitOperatorScalesEachAxisByItsCellExtent);
  testMethod(testTheTimeAveragedStateGrowsByTheGivenFactor);
  testMethod(testTheNextDerivativeIsTheSourceMinusTheFluxDivergence);
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheSeedCopiesTheCellStateAndClearsThePadding() {
  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int entry = 0; entry < CellLayout::NodesPerCell * CellLayout::NumberOfData; entry++) {
    cellState[entry] = 1.0 + 0.125 * entry;
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = MarkerValue;
  }

  exahype2::dg::ader::
    initialiseLinearPredictorState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      state,
      cellState
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        state[cellNode * CellLayout::PaddedNumberOfData + value],
        cellState[cellNode * CellLayout::NumberOfData + value],
        cellNode,
        value
      );
    }
    for (int value = CellLayout::NumberOfData; value < CellLayout::PaddedNumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        state[cellNode * CellLayout::PaddedNumberOfData + value],
        0.0,
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheTimeAverageStartsFromTheState() {
  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 1.0 + 0.5 * entry;
  }

  double timeAveragedState[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    timeAveragedState[entry] = MarkerValue;
  }

  exahype2::dg::ader::seedTimeAveragedState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
    timeAveragedState,
    state
  );

  for (int entry = 0; entry < StateSize; entry++) {
    validateNumericalEqualsWithParams1(timeAveragedState[entry], state[entry], entry);
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheFluxesAreClearedBeforeATaylorStep() {
  double flux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    flux[entry] = MarkerValue;
  }

  exahype2::dg::ader::clearFluxes<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(flux);

  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    validateNumericalEqualsWithParams1(flux[entry], 0.0, entry);
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testALinearFluxDifferentiatesToItsPhysicalSlope() {
  double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeDerivativeOperator(derivativeOperator);

  const auto inverseCellSize = makeInverseCellSize();

  double flux[DIMENSIONS * AxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double reference = QuadratureNodes[nodeIndexOf(cellNode)(axis)];
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] =
          1.0 + unknown + slopeOfAxis(axis) * reference;
      }
    }
  }

  exahype2::dg::ader::
    differentiateFluxInPlace<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      flux,
      derivativeOperator,
      inverseCellSize
    );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int base = axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          slopeOfAxis(axis) * inverseCellSize(axis),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < CellLayout::PaddedNumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, axis, cellNode, unknown);
      }
    }
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheUnitOperatorScalesEachAxisByItsCellExtent() {
  double unitOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeUnitOperator(unitOperator);

  const auto inverseCellSize = makeInverseCellSize();

  double flux[DIMENSIONS * AxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 1.0 + cellNode + unknown;
      }
    }
  }

  exahype2::dg::ader::
    differentiateFluxInPlace<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      flux,
      unitOperator,
      inverseCellSize
    );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
          (1.0 + cellNode + unknown) * inverseCellSize(axis),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheTimeAveragedStateGrowsByTheGivenFactor() {
  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 1.0 + 0.125 * entry;
  }

  double timeAveragedState[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    timeAveragedState[entry] = MarkerValue;
  }

  exahype2::dg::ader::
    accumulateTimeAveragedState<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      timeAveragedState,
      state,
      0.5
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int entry = cellNode * CellLayout::PaddedNumberOfData + unknown;
      validateNumericalEqualsWithParams2(timeAveragedState[entry], MarkerValue + 0.5 * state[entry], cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < CellLayout::PaddedNumberOfData; value++) {
      const int entry = cellNode * CellLayout::PaddedNumberOfData + value;
      validateNumericalEqualsWithParams2(timeAveragedState[entry], MarkerValue, cellNode, value);
    }
  }
}

void exahype2::dg::ader::tests::CauchyKovalewskiTest::testTheNextDerivativeIsTheSourceMinusTheFluxDivergence() {
  double flux[DIMENSIONS * AxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 1.0 + axis;
      }
    }
  }

  double sumOverAxes = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    sumOverAxes += 1.0 + axis;
  }

  double withSource[StateSize];
  double withoutSource[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    withSource[entry]    = MarkerValue;
    withoutSource[entry] = MarkerValue;
  }

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  exahype2::dg::ader::overwriteStateWithLinearSource<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    double>(withSource, cellCentre, cellSize, TimeStamp, TimeStepSize);
  exahype2::dg::ader::
    addFluxesToTimeDerivative<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, double>(
      withSource,
      flux
    );

  exahype2::dg::ader::
    addFluxesToTimeDerivative<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, false, double>(
      withoutSource,
      flux
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int entry = cellNode * CellLayout::PaddedNumberOfData + unknown;
      validateNumericalEqualsWithParams2(withSource[entry], 4.0 - sumOverAxes, cellNode, unknown);
      validateNumericalEqualsWithParams2(withoutSource[entry], -sumOverAxes, cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < CellLayout::PaddedNumberOfData; value++) {
      const int entry = cellNode * CellLayout::PaddedNumberOfData + value;
      validateNumericalEqualsWithParams2(withSource[entry], MarkerValue, cellNode, value);
    }
  }
}
