// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BoundaryExtrapolationTest.h"

#include "exahype2/dg/BoundaryExtrapolation.h"
#include "peano4/Constants.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int NodesPerAxisIn3D           = (DIMENSIONS == 3) ? NumberOfDofsPerAxis : 1;
  constexpr int NodesPerFace               = NumberOfDofsPerAxis * NodesPerAxisIn3D;
  constexpr int NodesPerCell               = NodesPerFace * NumberOfDofsPerAxis;

  struct FluxSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      int                                                           direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] += (direction + 1.0) * state[unknown];
      }
    }
  };

  // The diffusive flux along every axis is the sum over the axes of one plus
  // the axis number times the gradient along that axis.
  struct DiffusiveFluxSolver final {
    static void flux(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      [[maybe_unused]] int                                          direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (1.0 + axis) * stateGradients[axis][unknown];
        }
      }
    }
  };

  struct FaceBuffers final {
    double state[peano4::FacesPerCell][NodesPerFace * NumberOfData]{};
    double flux[peano4::FacesPerCell][NodesPerFace * NumberOfUnknowns]{};
  };

  // The entry (i, j) of the stub derivative operator differs for every pair,
  // so a transposed or shifted read changes the result.
  struct DerivativeOperator final {
    double entries[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};

    DerivativeOperator() {
      for (int row = 0; row < NumberOfDofsPerAxis; row++) {
        for (int column = 0; column < NumberOfDofsPerAxis; column++) {
          entries[row * NumberOfDofsPerAxis + column] = 0.4 * (column - row) + 0.1 * row;
        }
      }
    }
  };

  void collectPointers(
    FaceBuffers& buffers,
    double* (&state)[peano4::FacesPerCell],
    double* (&flux)[peano4::FacesPerCell]
  ) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      state[face] = buffers.state[face];
      flux[face]  = buffers.flux[face];
    }
  }

  // Returns the cell node that carries the given face node at the given index
  // along the normal axis.
  int cellNodeOf(const int direction, const int faceNode, const int normalIndex) {
    int lowerAxesStride = 1;
    for (int axis = 0; axis < direction; axis++) {
      lowerAxesStride *= NumberOfDofsPerAxis;
    }
    return ((faceNode / lowerAxesStride) * NumberOfDofsPerAxis + normalIndex) * lowerAxesStride
      + faceNode % lowerAxesStride;
  }
} // namespace

exahype2::dg::tests::BoundaryExtrapolationTest::BoundaryExtrapolationTest() :
  tarch::tests::TestCase("exahype2::dg::tests::BoundaryExtrapolationTest") {}

void exahype2::dg::tests::BoundaryExtrapolationTest::run() {
  testMethod(testFaceBuffersAreClearedBeforeTheProjection);
  testMethod(testAConstantStateReachesEveryFace);
  testMethod(testEachFaceNodeSeesOnlyItsOwnLine);
  testMethod(testTheTwoEndsUseTheirOwnCoefficients);
  testMethod(testFluxIsEvaluatedAtCellNodesBeforeExtrapolation);
  testMethod(testASystemWithoutFluxLeavesZeroesBehind);
  testMethod(testFluxOnAFaceComesFromTheFaceState);
  testMethod(testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal);
  testMethod(testDiffusiveFluxIsExtrapolatedWithTheNodalGradients);
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testFaceBuffersAreClearedBeforeTheProjection() {
  FaceBuffers buffers;
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      buffers.state[face][entry] = 9.0;
    }
    for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
      buffers.flux[face][entry] = 9.0;
    }
  }

  double* state[peano4::FacesPerCell];
  double* flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  const double                                cellState[NodesPerCell * NumberOfData] = {};
  const double                                coefficients[NumberOfDofsPerAxis]      = {};
  const double                                quadratureNodes[NumberOfDofsPerAxis]   = {};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::
    extrapolateStateToFaces<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      state,
      cellState,
      coefficients,
      coefficients
    );
  exahype2::dg::
    extrapolateFluxToFaces<FluxSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, false>(
      flux,
      cellState,
      coefficients,
      coefficients,
      quadratureNodes,
      coefficients,
      cellCentre,
      cellSize,
      0.0,
      0.0
    );

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      validateNumericalEquals(buffers.state[face][entry], 0.0);
    }
    for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEquals(buffers.flux[face][entry], 0.0);
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testAConstantStateReachesEveryFace() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  double cellState[NodesPerCell * NumberOfData] = {};
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 + value;
    }
  }

  double leftCoefficients[NumberOfDofsPerAxis];
  double rightCoefficients[NumberOfDofsPerAxis];
  double leftSum  = 0.0;
  double rightSum = 0.0;
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    leftCoefficients[index]  = 0.5 + 0.25 * index;
    rightCoefficients[index] = 1.5 - 0.125 * index;
    leftSum += leftCoefficients[index];
    rightSum += rightCoefficients[index];
  }

  exahype2::dg::
    extrapolateStateToFaces<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      state,
      cellState,
      leftCoefficients,
      rightCoefficients
    );

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        validateNumericalEquals(state[direction][faceNode * NumberOfData + value], (1.0 + value) * leftSum);
        validateNumericalEquals(
          state[direction + DIMENSIONS][faceNode * NumberOfData + value],
          (1.0 + value) * rightSum
        );
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testEachFaceNodeSeesOnlyItsOwnLine() {
  double leftCoefficients[NumberOfDofsPerAxis];
  double rightCoefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    leftCoefficients[index]  = 1.0 + index;
    rightCoefficients[index] = 0.0;
  }

  for (int markedCellNode = 0; markedCellNode < NodesPerCell; markedCellNode++) {
    FaceBuffers buffers;
    double*     state[peano4::FacesPerCell];
    double*     flux[peano4::FacesPerCell];
    collectPointers(buffers, state, flux);

    double    cellState[NodesPerCell * NumberOfData] = {};
    const int markedEntry                            = markedCellNode * NumberOfData;
    cellState[markedEntry]                           = 1.0;

    exahype2::dg::
      extrapolateStateToFaces<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
        state,
        cellState,
        leftCoefficients,
        rightCoefficients
      );

    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          if (cellNodeOf(direction, faceNode, normalIndex) == markedCellNode) {
            expected = leftCoefficients[normalIndex];
          }
        }
        const int faceEntry = faceNode * NumberOfData;
        validateNumericalEquals(state[direction][faceEntry], expected);
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testTheTwoEndsUseTheirOwnCoefficients() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  double cellState[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    cellState[entry] = 1.0 + 0.5 * entry;
  }

  double leftCoefficients[NumberOfDofsPerAxis];
  double rightCoefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    leftCoefficients[index]  = 1.0 + index;
    rightCoefficients[index] = 3.0 * (1.0 + index);
  }

  exahype2::dg::
    extrapolateStateToFaces<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      state,
      cellState,
      leftCoefficients,
      rightCoefficients
    );

  // The upper coefficients are three times the lower ones, so the upper face
  // carries three times the value of the lower one.
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      validateNumericalEquals(state[direction + DIMENSIONS][entry], 3.0 * state[direction][entry]);
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testFluxIsEvaluatedAtCellNodesBeforeExtrapolation() {
  double coefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    coefficients[index] = 1.0 + index;
  }

  double cellState[NodesPerCell * NumberOfData] = {};
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      cellState[cellNode * NumberOfData + unknown] = 1.0 + cellNode + 0.25 * unknown;
    }
  }

  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);
  const double                                quadratureNodes[NumberOfDofsPerAxis] = {};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::
    extrapolateFluxToFaces<FluxSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, false>(
      flux,
      cellState,
      coefficients,
      coefficients,
      quadratureNodes,
      coefficients,
      cellCentre,
      cellSize,
      0.0,
      0.0
    );

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int cellNode = cellNodeOf(direction, faceNode, normalIndex);
          expected += (direction + 1.0) * cellState[cellNode * NumberOfData + unknown] * coefficients[normalIndex];
        }
        validateNumericalEquals(flux[direction][faceNode * NumberOfUnknowns + unknown], expected);
        validateNumericalEquals(flux[direction + DIMENSIONS][faceNode * NumberOfUnknowns + unknown], expected);
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testASystemWithoutFluxLeavesZeroesBehind() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  double cellState[NodesPerCell * NumberOfData] = {};
  double coefficients[NumberOfDofsPerAxis];
  double quadratureNodes[NumberOfDofsPerAxis] = {};
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    coefficients[index] = 1.0 + index;
  }
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::
    extrapolateFluxToFaces<FluxSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, false, false>(
      flux,
      cellState,
      coefficients,
      coefficients,
      quadratureNodes,
      coefficients,
      cellCentre,
      cellSize,
      0.0,
      0.0
    );

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEquals(flux[face][entry], 0.0);
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testFluxOnAFaceComesFromTheFaceState() {
  const double                                quadratureNodes[NumberOfDofsPerAxis] = {0.1, 0.5, 0.9};
  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
    leftState[entry]  = 1.0 + 0.5 * entry;
    rightState[entry] = -2.0 + 0.25 * entry;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int side = 0; side < 2; side++) {
      const double* const faceState = side == 0 ? leftState : rightState;
      double              faceFlux[NodesPerFace * NumberOfUnknowns];
      for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
        faceFlux[entry] = 9.0;
      }

      exahype2::dg::
        evaluateFluxOnFace<FluxSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, false>(
          faceFlux,
          leftState,
          rightState,
          side,
          quadratureNodes,
          faceCentre,
          cellSize,
          0.0,
          0.0,
          direction
        );

      // The accumulating stub starts from a cleared flux, so every node holds the
      // stub's multiple of the state of the selected side.
      for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams3(
            faceFlux[faceNode * NumberOfUnknowns + unknown],
            (direction + 1.0) * faceState[faceNode * NumberOfData + unknown],
            direction,
            side,
            faceNode
          );
        }
      }

      exahype2::dg::
        evaluateFluxOnFace<FluxSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, false, false>(
          faceFlux,
          leftState,
          rightState,
          side,
          quadratureNodes,
          faceCentre,
          cellSize,
          0.0,
          0.0,
          direction
        );
      for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
        validateNumericalEquals(faceFlux[entry], 0.0);
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal() {
  const double                                quadratureNodes[NumberOfDofsPerAxis] = {0.1, 0.5, 0.9};
  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  tarch::la::Vector<DIMENSIONS, double>       cellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellSize(axis) = 0.5 + 0.25 * axis;
  }

  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
    leftState[entry]  = 1.0 + 0.5 * entry;
    rightState[entry] = 4.0 - 0.25 * entry;
  }

  // Both sides see the same jump, so both fluxes hold the stub's multiple of
  // the jump over the cell extent along the normal axis.
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int side = 0; side < 2; side++) {
      double faceFlux[NodesPerFace * NumberOfUnknowns] = {};
      exahype2::dg::evaluateFluxOnFace<
        DiffusiveFluxSolver,
        NumberOfDofsPerAxis,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        true,
        true>(faceFlux, leftState, rightState, side, quadratureNodes, faceCentre, cellSize, 0.0, 0.0, direction);
      for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          const int    entry    = faceNode * NumberOfData + unknown;
          const double expected = (direction + 1.0) * (rightState[entry] - leftState[entry]) / cellSize(direction);
          validateNumericalEqualsWithParams3(
            faceFlux[faceNode * NumberOfUnknowns + unknown],
            expected,
            direction,
            side,
            faceNode
          );
        }
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testDiffusiveFluxIsExtrapolatedWithTheNodalGradients() {
  const DerivativeOperator derivativeOperator;
  double                   coefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    coefficients[index] = 1.0 + index;
  }
  const double                                quadratureNodes[NumberOfDofsPerAxis] = {0.1, 0.5, 0.9};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  tarch::la::Vector<DIMENSIONS, double>       cellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellSize(axis) = 0.5 + 0.25 * axis;
  }

  double cellState[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    cellState[entry] = 1.0 + 0.5 * entry - 0.01 * entry * entry;
  }

  // The reference gradient applies the operator line by line, so the kernel's
  // block layout is checked against a direct evaluation.
  double gradients[DIMENSIONS][NodesPerCell * NumberOfData] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int line = 0; line < NodesPerFace; line++) {
      for (int targetIndex = 0; targetIndex < NumberOfDofsPerAxis; targetIndex++) {
        const int targetNode = cellNodeOf(axis, line, targetIndex);
        for (int sourceIndex = 0; sourceIndex < NumberOfDofsPerAxis; sourceIndex++) {
          const int    sourceNode = cellNodeOf(axis, line, sourceIndex);
          const double weight =
            derivativeOperator.entries[targetIndex * NumberOfDofsPerAxis + sourceIndex] / cellSize(axis);
          for (int value = 0; value < NumberOfData; value++) {
            gradients[axis][targetNode * NumberOfData + value] += weight * cellState[sourceNode * NumberOfData + value];
          }
        }
      }
    }
  }

  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);
  exahype2::dg::extrapolateFluxToFaces<
    DiffusiveFluxSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true>(
    flux,
    cellState,
    coefficients,
    coefficients,
    quadratureNodes,
    derivativeOperator.entries,
    cellCentre,
    cellSize,
    0.0,
    0.0
  );

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int cellNode = cellNodeOf(direction, faceNode, normalIndex);
          double    nodeFlux = 0.0;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            nodeFlux += (1.0 + axis) * gradients[axis][cellNode * NumberOfData + unknown];
          }
          expected += nodeFlux * coefficients[normalIndex];
        }
        validateNumericalEqualsWithParams3(
          flux[direction][faceNode * NumberOfUnknowns + unknown],
          expected,
          direction,
          faceNode,
          unknown
        );
        validateNumericalEqualsWithParams3(
          flux[direction + DIMENSIONS][faceNode * NumberOfUnknowns + unknown],
          expected,
          direction,
          faceNode,
          unknown
        );
      }
    }
  }
}
