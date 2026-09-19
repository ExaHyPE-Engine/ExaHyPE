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
  constexpr int NodesPerAxisIn3D           = (DIMENSIONS > 2) ? NumberOfDofsPerAxis : 1;
  constexpr int NodesPerFace               = NumberOfDofsPerAxis * NodesPerAxisIn3D;
  constexpr int NodesPerCell               = NodesPerFace * NumberOfDofsPerAxis;
  constexpr int PaddedNodeStride           = NumberOfData + 2;

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

  // The lowest tangential axis of the face node runs fastest.
  int cellNodeInAxisOrder(const int direction, const int faceNode, const int normalIndex) {
    tarch::la::Vector<DIMENSIONS, int> nodeIndex(0);
    nodeIndex(direction) = normalIndex;
    int remainder        = faceNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis != direction) {
        nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
        remainder /= NumberOfDofsPerAxis;
      }
    }
    int cellNode = 0;
    int stride   = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellNode += nodeIndex(axis) * stride;
      stride *= NumberOfDofsPerAxis;
    }
    return cellNode;
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
  testMethod(testAPaddedNodeStrideProjectsOnlyTheEntries);
  testMethod(testTheLowerAndUpperFaceUseTheirOwnCoefficients);
  testMethod(testTheFaceNodesRunWithTheLowestTangentialAxisFastest);
  testMethod(testAFieldOfFloatAccumulatesTheFacesInDouble);
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

void exahype2::dg::tests::BoundaryExtrapolationTest::testAPaddedNodeStrideProjectsOnlyTheEntries() {
  // The lanes behind the entries of a node carry a value that would dominate every face if the
  // projection read them.
  double field[NodesPerCell * PaddedNodeStride];
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int lane = 0; lane < PaddedNodeStride; lane++) {
      field[cellNode * PaddedNodeStride + lane] = (lane < NumberOfData) ? (1.0 + cellNode + 0.25 * lane) : 1000.0;
    }
  }

  double coefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    coefficients[index] = 1.0;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double lowerFace[NodesPerFace * NumberOfData];
    double upperFace[NodesPerFace * NumberOfData];
    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      lowerFace[entry] = 9.0;
      upperFace[entry] = 9.0;
    }

    exahype2::dg::
      extrapolateNodalFieldToFacePair<NumberOfDofsPerAxis, NumberOfData, PaddedNodeStride, double, double, double>(
        lowerFace,
        upperFace,
        field,
        coefficients,
        coefficients,
        direction
      );

    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int cellNode = cellNodeInAxisOrder(direction, faceNode, normalIndex);
          expected += field[cellNode * PaddedNodeStride + entry];
        }
        validateNumericalEqualsWithParams3(
          lowerFace[faceNode * NumberOfData + entry],
          expected,
          direction,
          faceNode,
          entry
        );
        validateNumericalEqualsWithParams3(
          upperFace[faceNode * NumberOfData + entry],
          expected,
          direction,
          faceNode,
          entry
        );
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testTheLowerAndUpperFaceUseTheirOwnCoefficients() {
  double field[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    field[entry] = 1.0 + 0.5 * entry;
  }

  double leftCoefficients[NumberOfDofsPerAxis];
  double rightCoefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    leftCoefficients[index]  = 1.0 + index;
    rightCoefficients[index] = 0.5 - 0.25 * index;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double lowerFace[NodesPerFace * NumberOfData] = {};
    double upperFace[NodesPerFace * NumberOfData] = {};

    exahype2::dg::
      extrapolateNodalFieldToFacePair<NumberOfDofsPerAxis, NumberOfData, NumberOfData, double, double, double>(
        lowerFace,
        upperFace,
        field,
        leftCoefficients,
        rightCoefficients,
        direction
      );

    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        double expectedLower = 0.0;
        double expectedUpper = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int    cellNode = cellNodeInAxisOrder(direction, faceNode, normalIndex);
          const double value    = field[cellNode * NumberOfData + entry];
          expectedLower += value * leftCoefficients[normalIndex];
          expectedUpper += value * rightCoefficients[normalIndex];
        }
        validateNumericalEqualsWithParams3(
          lowerFace[faceNode * NumberOfData + entry],
          expectedLower,
          direction,
          faceNode,
          entry
        );
        validateNumericalEqualsWithParams3(
          upperFace[faceNode * NumberOfData + entry],
          expectedUpper,
          direction,
          faceNode,
          entry
        );
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testTheFaceNodesRunWithTheLowestTangentialAxisFastest() {
  // Every cell node carries its own value, and a coefficient vector that holds one at a single index
  // hands the value of that node's line position to the face node.
  double field[NodesPerCell * NumberOfData];
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int entry = 0; entry < NumberOfData; entry++) {
      field[cellNode * NumberOfData + entry] = 1.0 + cellNode + 0.125 * entry;
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int markedIndex = 0; markedIndex < NumberOfDofsPerAxis; markedIndex++) {
      double leftCoefficients[NumberOfDofsPerAxis]             = {};
      double rightCoefficients[NumberOfDofsPerAxis]            = {};
      leftCoefficients[markedIndex]                            = 1.0;
      rightCoefficients[NumberOfDofsPerAxis - 1 - markedIndex] = 1.0;

      double lowerFace[NodesPerFace * NumberOfData] = {};
      double upperFace[NodesPerFace * NumberOfData] = {};

      exahype2::dg::
        extrapolateNodalFieldToFacePair<NumberOfDofsPerAxis, NumberOfData, NumberOfData, double, double, double>(
          lowerFace,
          upperFace,
          field,
          leftCoefficients,
          rightCoefficients,
          direction
        );

      for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
        const int lowerCellNode = cellNodeInAxisOrder(direction, faceNode, markedIndex);
        const int upperCellNode = cellNodeInAxisOrder(direction, faceNode, NumberOfDofsPerAxis - 1 - markedIndex);
        for (int entry = 0; entry < NumberOfData; entry++) {
          validateNumericalEqualsWithParams3(
            lowerFace[faceNode * NumberOfData + entry],
            field[lowerCellNode * NumberOfData + entry],
            direction,
            markedIndex,
            faceNode
          );
          validateNumericalEqualsWithParams3(
            upperFace[faceNode * NumberOfData + entry],
            field[upperCellNode * NumberOfData + entry],
            direction,
            markedIndex,
            faceNode
          );
        }
      }
    }
  }
}

void exahype2::dg::tests::BoundaryExtrapolationTest::testAFieldOfFloatAccumulatesTheFacesInDouble() {
  // The small value is half the distance between the large value and its float neighbour, so a sum
  // taken in float drops it and a sum taken in double keeps it.
  constexpr double LargeValue = 1024.0;
  constexpr double SmallValue = 0.00006103515625;

  double leftCoefficients[NumberOfDofsPerAxis];
  double rightCoefficients[NumberOfDofsPerAxis];
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    leftCoefficients[index]  = 1.0;
    rightCoefficients[index] = (index + 1 < NumberOfDofsPerAxis) ? 1.0 : 0.0;
  }

  double expectedLower = 0.0;
  double expectedUpper = 0.0;
  for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
    const double value = (normalIndex == 0) ? LargeValue : SmallValue;
    expectedLower += value * leftCoefficients[normalIndex];
    expectedUpper += value * rightCoefficients[normalIndex];
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    float field[NodesPerCell * NumberOfData];
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
        const int  cellNode = cellNodeInAxisOrder(direction, faceNode, normalIndex);
        const auto value    = static_cast<float>((normalIndex == 0) ? LargeValue : SmallValue);
        for (int entry = 0; entry < NumberOfData; entry++) {
          field[cellNode * NumberOfData + entry] = value;
        }
      }
    }

    double lowerFace[NodesPerFace * NumberOfData] = {};
    double upperFace[NodesPerFace * NumberOfData] = {};

    exahype2::dg::
      extrapolateNodalFieldToFacePair<NumberOfDofsPerAxis, NumberOfData, NumberOfData, float, double, double>(
        lowerFace,
        upperFace,
        field,
        leftCoefficients,
        rightCoefficients,
        direction
      );

    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      validateNumericalEqualsWithParams2(lowerFace[entry], expectedLower, direction, entry);
      validateNumericalEqualsWithParams2(upperFace[entry], expectedUpper, direction, entry);
    }
  }
}
