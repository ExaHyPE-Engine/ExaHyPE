// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/Kernels.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/TransferTestData.h"

namespace {
  using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

  TARGET_DECLARE
  struct Order2Layout {
    using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

    static constexpr int DGOrder                    = 2;
    static constexpr int NumberOfUnknowns           = 2;
    static constexpr int NumberOfAuxiliaryVariables = 1;
  };

  constexpr int NodesPerAxis     = Order2Layout::DGOrder + 1;
  constexpr int NumberOfUnknowns = Order2Layout::NumberOfUnknowns;
  constexpr int NumberOfData     = NumberOfUnknowns + Order2Layout::NumberOfAuxiliaryVariables;
  constexpr int FaceNodes        = exahype2::dg::getNodesPerFace(NodesPerAxis);
  constexpr int CellNodes        = exahype2::dg::getNodesPerCell(NodesPerAxis);
  TARGET_DECLARE_END

  TARGET_DECLARE
  /**
   * A flux that writes the state times one plus the axis number.
   */
  struct FluxSolver final : Order2Layout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      int                                                           direction,
      double* NOALIAS const                                         fluxValues,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (direction + 1.0) * state[unknown];
      }
    }
  };

  struct DiffusiveFluxSolver final : Order2Layout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      [[maybe_unused]] int                                          direction,
      double* NOALIAS const                                         fluxValues,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (1.0 + axis) * stateGradients[axis][unknown];
        }
      }
    }
  };

  /**
   * Writes the inner state, the position and the time stamp into the outer state.
   */
  struct RecordingSolver final : Order2Layout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      const int                                                     direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int value = 0; value < NumberOfData; value++) {
        outsideState[value] =
          2.0 * insideState[value] + position(direction) + 10.0 * position((direction + 1) % DIMENSIONS) + timeStamp;
      }
    }
  };
  TARGET_DECLARE_END

  struct FaceBuffers final {
    double state[peano4::FacesPerCell][FaceNodes * NumberOfData]{};
    double flux[peano4::FacesPerCell][FaceNodes * NumberOfUnknowns]{};
  };

  // The nodes of the three-point Gauss-Legendre rule on the unit interval.
  const double QuadratureNodes[NodesPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};

  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  double getBasisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < NodesPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < NodesPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  /**
   * The values of the basis functions at the two ends of the unit interval.
   */
  struct EndCoefficients final {
    double left[NodesPerAxis]{};
    double right[NodesPerAxis]{};

    EndCoefficients() {
      for (int node = 0; node < NodesPerAxis; node++) {
        left[node]  = getBasisValue(0.0, node);
        right[node] = getBasisValue(1.0, node);
      }
    }
  };

  /**
   * The entry (i, j) is the derivative of basis function j at node i.
   */
  struct DerivativeOperator final {
    double entries[NodesPerAxis * NodesPerAxis]{};

    DerivativeOperator() {
      for (int row = 0; row < NodesPerAxis; row++) {
        for (int column = 0; column < NodesPerAxis; column++) {
          entries[row * NodesPerAxis + column] = getBasisDerivative(QuadratureNodes[row], column);
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

  /**
   * Returns the cell node of face node faceNode at position normalIndex along direction.
   */
  int getCellNodeOf(const int direction, const int faceNode, const int normalIndex) {
    int lowerAxesStride = 1;
    for (int axis = 0; axis < direction; axis++) {
      lowerAxesStride *= NodesPerAxis;
    }
    return ((faceNode / lowerAxesStride) * NodesPerAxis + normalIndex) * lowerAxesStride + faceNode % lowerAxesStride;
  }

  constexpr double BoundaryTimeStamp                     = 0.75;
  constexpr double BoundaryQuadratureNodes[NodesPerAxis] = {0.1, 0.5, 0.9};

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  tarch::la::Vector<DIMENSIONS, double> getFaceCentre(const int faceNumber) {
    const auto                            cellSize = getCellSize();
    tarch::la::Vector<DIMENSIONS, double> faceCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      faceCentre(axis) = 1.0 + axis;
    }
    faceCentre(faceNumber % DIMENSIONS) += (faceNumber < DIMENSIONS ? -0.5 : 0.5) * cellSize(faceNumber % DIMENSIONS);
    return faceCentre;
  }

  tarch::la::Vector<DIMENSIONS, double> getNodePositionOf(const int faceNumber, const int faceNode) {
    const int  direction  = faceNumber % DIMENSIONS;
    const auto cellSize   = getCellSize();
    const auto faceCentre = getFaceCentre(faceNumber);
    auto       position   = faceCentre;
    const auto nodeIndex  = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis != direction) {
        position(axis) = faceCentre(axis) + (BoundaryQuadratureNodes[nodeIndex(axis)] - 0.5) * cellSize(axis);
      }
    }
    return position;
  }

  /**
   * Returns the half of the boundary face faceNumber that the cell inside the domain writes.
   */
  int getInnerSideOf(const int faceNumber) { return faceNumber < DIMENSIONS ? 1 : 0; }

  void fillInnerHalf(double (&faceState)[2 * FaceNodes * NumberOfData], const int innerSide) {
    for (int entry = 0; entry < 2 * FaceNodes * NumberOfData; entry++) {
      faceState[entry] = -1.0;
    }
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      faceState[innerSide * FaceNodes * NumberOfData + entry] = 0.25 * entry;
    }
  }

  TARGET_DECLARE
  template <int Auxiliary>
  struct Order1Layout {
    using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 2;
    static constexpr int NumberOfAuxiliaryVariables = Auxiliary;
  };

  constexpr int RiemannUnknowns  = Order1Layout<0>::NumberOfUnknowns;
  constexpr int RiemannFaceNodes = exahype2::dg::getNodesPerFace(Order1Layout<0>::DGOrder + 1);
  TARGET_DECLARE_END

  TARGET_DECLARE
  /**
   * Returns the first entry of the state as the wave speed. A test sets the wave speed of each side
   * through that entry.
   */
  struct FirstEntryEigenvalue {
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return state[0];
    }
  };

  /**
   * A flux that adds the state. A flux buffer that the kernel does not clear changes the result.
   */
  struct AccumulatingFlux {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         fluxValues,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        fluxValues[unknown] += state[unknown];
      }
    }
  };

  struct JumpProduct {
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         productValues,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        productValues[unknown] += stateJump[unknown];
      }
    }
  };

  /**
   * A nonconservative product that reads the jump of the first auxiliary variable.
   */
  struct AuxiliaryJumpProduct {
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         productValues,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        productValues[unknown] += (1.0 + unknown) * stateJump[RiemannUnknowns];
      }
    }
  };

  /**
   * Doubles the values. The doubled values show that the matrix ran.
   */
  struct DoublingMaterial {
    static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        values[unknown] *= 2.0;
      }
    }
  };

  struct EigenvalueSolver final : Order1Layout<0>, FirstEntryEigenvalue {};
  struct EigenvalueSolverWithAuxiliary final : Order1Layout<1>, FirstEntryEigenvalue {};
  struct ProductSolver final : Order1Layout<0>, FirstEntryEigenvalue, JumpProduct {};
  struct ProductSolverWithAuxiliary final : Order1Layout<1>, FirstEntryEigenvalue, JumpProduct {};
  struct AuxiliaryJumpSolver final : Order1Layout<1>, FirstEntryEigenvalue, AuxiliaryJumpProduct {};
  struct LinearFluxSolver final : Order1Layout<0>, FirstEntryEigenvalue, AccumulatingFlux {};
  struct LinearFluxProductSolver final : Order1Layout<0>, FirstEntryEigenvalue, AccumulatingFlux, JumpProduct {};
  struct LinearFluxMaterialSolver final : Order1Layout<0>, FirstEntryEigenvalue, AccumulatingFlux, DoublingMaterial {};
  TARGET_DECLARE_END

  /**
   * Writes weights that sum to one. A uniform state averages to itself.
   */
  void fillNormalisedWeights(double (&faceQuadratureWeights)[RiemannFaceNodes]) {
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      faceQuadratureWeights[faceNode] = 1.0 / RiemannFaceNodes;
    }
  }

  TARGET_DECLARE
  constexpr int    FaceUnknowns  = 1;
  constexpr int    FaceData      = FaceUnknowns;
  constexpr double CellExtent    = 2.0;
  constexpr double TimeStamp     = 0.25;
  constexpr double TimeStepSize  = 0.5;
  constexpr double WaveSpeed     = 3.0;
  constexpr double OutsideValue  = 5.0;
  constexpr int    FaceStateSize = RiemannFaceNodes * FaceData;
  constexpr int    FaceFluxSize  = RiemannFaceNodes * FaceUnknowns;
  constexpr double LowerOwnFlux  = 10.0;
  constexpr double UpperOwnFlux  = 20.0;

  /**
   * A solver layout whose IsLinear selects the Riemann solver.
   */
  template <bool Linear>
  struct FaceLayout {
    using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

    static constexpr int  DGOrder                    = 1;
    static constexpr int  NumberOfUnknowns           = FaceUnknowns;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = Linear;
  };

  /**
   * Returns one number whose decimal digits encode the arguments a face kernel passes to the
   * solver.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD double encodeFaceArguments(
    const double timeStamp,
    const double timeStepSize,
    const int    normal,
    const bool   isBoundaryFace,
    const int    faceIndex
  ) {
    return timeStamp + 10.0 * timeStepSize + 100.0 * normal + (isBoundaryFace ? 1000.0 : 0.0) + 10000.0 * faceIndex;
  }

  /**
   * Terms with a wave speed that is nonzero only at the time stamp of the tests and a constant
   * outside state.
   */
  struct FaceTerms {
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return timeStamp == TimeStamp ? WaveSpeed : 0.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      [[maybe_unused]] const double* NOALIAS const                  insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int value = 0; value < FaceData; value++) {
        outsideState[value] = OutsideValue;
      }
    }
  };

  struct FaceSolver final : FaceLayout<false>, FaceTerms {};
  struct LinearFaceSolver final : FaceLayout<true>, FaceTerms {};

  /**
   * A solver whose Riemann solver adds the encoded face arguments to the fluxes of both sides.
   */
  struct FaceSolverWithRiemannSolver final : FaceLayout<false>, FaceTerms {
    static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
      [[maybe_unused]] const double* NOALIAS const                  leftState,
      [[maybe_unused]] const double* NOALIAS const                  rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      const double                                                  timeStepSize,
      const int                                                     normal,
      const bool                                                    isBoundaryFace,
      const int                                                     faceIndex,
      double* NOALIAS const                                         leftFlux,
      double* NOALIAS const                                         rightFlux,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      const double increment = encodeFaceArguments(timeStamp, timeStepSize, normal, isBoundaryFace, faceIndex);
      for (int entry = 0; entry < FaceFluxSize; entry++) {
        leftFlux[entry] += increment;
        rightFlux[entry] += increment;
      }
      return increment;
    }
  };
  TARGET_DECLARE_END

  struct Face final {
    double faceState[2 * FaceStateSize]{};
    double faceFlux[2 * FaceFluxSize]{};
  };

  void fill(Face& face) {
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      face.faceState[faceNode]                 = 2.0 + faceNode;
      face.faceState[FaceStateSize + faceNode] = 1.0;
      face.faceFlux[faceNode]                  = LowerOwnFlux + faceNode;
      face.faceFlux[FaceFluxSize + faceNode]   = UpperOwnFlux;
    }
  }

  template <typename Solver>
  void solveFace(Face& face, const int direction, const int boundaryFaceNumber) {
    const double quadraturePoints[2] = {0.0, 1.0};
    double       faceQuadratureWeights[RiemannFaceNodes];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      faceQuadratureWeights[faceNode] = 1.0;
    }
    exahype2::dg::solveRiemannProblemOnFace<Solver>(
      face.faceState,
      face.faceFlux,
      quadraturePoints,
      faceQuadratureWeights,
      tarch::la::Vector<DIMENSIONS, double>(0.5 * CellExtent),
      tarch::la::Vector<DIMENSIONS, double>(CellExtent),
      TimeStamp,
      TimeStepSize,
      direction,
      boundaryFaceNumber,
      RuntimeParameters{}
    );
  }

  /**
   * A solver layout without NumberOfAuxiliaryVariables. A face integral does not read that constant.
   */
  struct FaceIntegralLayout final {
    static constexpr int DGOrder          = 2;
    static constexpr int NumberOfUnknowns = 2;
  };

  struct Tables final {
    double faceQuadratureWeights[FaceNodes]{};
    double inverseCellQuadratureWeights[CellNodes]{};
    double leftFaceCoefficients[NodesPerAxis]{};
    double rightFaceCoefficients[NodesPerAxis]{};
  };

  // The entries are distinct and non-symmetric. An index mix-up does not cancel out.
  Tables getTables() {
    Tables tables;
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      tables.faceQuadratureWeights[faceNode] = 0.25 + 0.125 * faceNode;
    }
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      tables.inverseCellQuadratureWeights[cellNode] = 1.0 + 0.5 * cellNode;
    }
    for (int index = 0; index < NodesPerAxis; index++) {
      tables.leftFaceCoefficients[index]  = 1.0 + index;
      tables.rightFaceCoefficients[index] = 4.0 + index;
    }
    return tables;
  }

  /**
   * Subtracts the integral of one face from the updates of every node in cellUpdate.
   */
  void addFaceIntegralToEveryNode(
    double* const       cellUpdate,
    const double* const faceFlux,
    const Tables&       tables,
    const int           direction,
    const int           orientation,
    const double        inverseCellSizeAlongNormal,
    const double        timeStepSize
  ) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      exahype2::dg::addFaceIntegral<FaceIntegralLayout>(
        cellUpdate + static_cast<std::ptrdiff_t>(cellNode) * NumberOfUnknowns,
        cellNode,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        direction,
        orientation,
        inverseCellSizeAlongNormal,
        timeStepSize
      );
    }
  }

  TARGET_DECLARE
  constexpr int EigenvalueCellNodes = exahype2::dg::getNodesPerCell(2);

  /**
   * A solver whose wave speed depends on the state, the direction and the node position.
   */
  struct EigenvalueStubSolver final {
    using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 1;

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* NOALIAS const                                   state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      const int                                                     direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return state[0] + direction + 0.25 * position(0);
    }
  };

  /**
   * A solver whose wave speed is the unknown of a node and whose postprocessing adds one to the
   * unknown of every node.
   */
  struct PostprocessingStubSolver final {
    using RuntimeParameters = exahype2::dg::unittests::RuntimeParameters;

    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 1;

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return state[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int cellNode = 0; cellNode < EigenvalueCellNodes; cellNode++) {
        Q[cellNode * (NumberOfUnknowns + NumberOfAuxiliaryVariables)] += 1.0;
      }
    }
  };
  TARGET_DECLARE_END

  using TransferSetup = exahype2::dg::unittests::TransferSetup;

  constexpr int ValuesPerTransferCell = TransferSetup::CellNodes * TransferSetup::ValuesPerNode;

  /**
   * A solver layout with the nodes of the transfer setup and every value an unknown.
   */
  struct TransferLayout final {
    static constexpr int DGOrder                    = TransferSetup::NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = TransferSetup::ValuesPerNode;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  using CellPool = tarch::accelerator::ChunkedMemoryPool<double>;
  using Transfer = exahype2::dg::CellTransfer<TransferLayout, double>;

  Transfer getTransferOf(const CellPool& pool, const exahype2::dg::unittests::TransferOperators& operators) {
    return Transfer{pool.getView(), operators.prolongation.data(), operators.restriction.data()};
  }

  using Setup = exahype2::dg::unittests::KernelTestSetup;
  using Stub  = exahype2::dg::unittests::StubSolver;

  TARGET_DECLARE
  /**
   * Writes the state of one cell into its own half of each of its faces.
   */
  struct ExtrapolateStateOfCell final {
    const double*        cellStates{nullptr};
    double*              faceStates{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const double*        leftFaceCoefficients{nullptr};
    const double*        rightFaceCoefficients{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      // The two cells of a face write disjoint halves.
      double* facesOfCell[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        facesOfCell[face] = faceStates + faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceStateCardinality
          + (face < DIMENSIONS ? Setup::FaceSideStateCardinality : 0);
      }
      const int cellStateOffset = cell * Setup::CellStateCardinality;
      exahype2::dg::extrapolateStateToFaces<Stub>(
        facesOfCell,
        cellStates + cellStateOffset,
        leftFaceCoefficients,
        rightFaceCoefficients
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getExtrapolatedStatesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      cellStates,
    const std::vector<double>&                      faceStates
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceRightFaceCoefficients(operators.rightFaceCoefficients);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      ExtrapolateStateOfCell{
        deviceCellStates.getData(),
        deviceFaceStates.getData(),
        deviceFaceIndices.getData(),
        deviceLeftFaceCoefficients.getData(),
        deviceRightFaceCoefficients.getData()
      }
    );

    std::vector<double> result(faceStates.size());
    deviceFaceStates.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  /**
   * Writes the flux of one cell into its own half of each of its faces.
   */
  struct ExtrapolateFluxOfCell final {
    const double*        cellStates{nullptr};
    double*              faceFluxes{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const double*        cellCentres{nullptr};
    const double*        cellSizes{nullptr};
    const double*        leftFaceCoefficients{nullptr};
    const double*        rightFaceCoefficients{nullptr};
    const double*        quadratureNodes{nullptr};
    const double*        derivativeOperator{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      double* facesOfCell[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        facesOfCell[face] = faceFluxes + faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceFluxCardinality
          + (face < DIMENSIONS ? Setup::FaceSideFluxCardinality : 0);
      }
      const int cellStateOffset  = cell * Setup::CellStateCardinality;
      const int coordinateOffset = cell * DIMENSIONS;
      double    nodalFluxes[DIMENSIONS * Setup::StageCardinality];
      exahype2::dg::evaluateNodalFluxes<Stub>(
        nodalFluxes,
        cellStates + cellStateOffset,
        quadratureNodes,
        derivativeOperator,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        RuntimeParameters{}
      );
      exahype2::dg::extrapolateFluxToFaces<Stub>(facesOfCell, nodalFluxes, leftFaceCoefficients, rightFaceCoefficients);
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getExtrapolatedFluxesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      cellStates,
    const std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceRightFaceCoefficients(operators.rightFaceCoefficients);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceQuadratureNodes(operators.quadratureNodes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceDerivativeOperator(operators.derivativeOperator);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      ExtrapolateFluxOfCell{
        deviceCellStates.getData(),
        deviceFaceFluxes.getData(),
        deviceFaceIndices.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceLeftFaceCoefficients.getData(),
        deviceRightFaceCoefficients.getData(),
        deviceQuadratureNodes.getData(),
        deviceDerivativeOperator.getData()
      }
    );

    std::vector<double> result(faceFluxes.size());
    deviceFaceFluxes.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  /**
   * Writes the flux of one side of one work face. The two sides write disjoint halves.
   */
  struct EvaluateFluxOnFaceSide final {
    const double*        faceStates{nullptr};
    double*              faceFluxes{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const std::uint8_t*  faceNumbers{nullptr};
    const std::uint64_t* cellIndices{nullptr};
    const double*        cellCentres{nullptr};
    const double*        cellSizes{nullptr};
    const double*        quadratureNodes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int item) const {
      const int   face             = item / 2;
      const int   side             = item % 2;
      const int   faceNumber       = faceNumbers[face];
      const int   cell             = cellIndices[face];
      const int   sideFluxOffset   = side * Setup::FaceSideFluxCardinality;
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = faceStates + faceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = faceFluxes + faceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::evaluateFluxOnFace<Stub>(
        faceFlux + sideFluxOffset,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        side,
        quadratureNodes,
        exahype2::dg::unittests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getFluxesOnFacesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      faceStates,
    const std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::unittests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(2 * geometry.getNumberOfWorkFaces()),
      EvaluateFluxOnFaceSide{
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceFaceIndices.getData(),
        deviceFaceNumbers.getData(),
        deviceCellIndices.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceQuadratureNodes.getData()
      }
    );

    std::vector<double> result(faceFluxes.size());
    deviceFaceFluxes.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  /**
   * Writes the boundary conditions of the solver into the outer half of one work face.
   */
  struct ApplyBoundaryConditionsOfFace final {
    double*              faceStates{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const std::uint8_t*  faceNumbers{nullptr};
    const std::uint64_t* cellIndices{nullptr};
    const double*        cellCentres{nullptr};
    const double*        cellSizes{nullptr};
    const double*        quadratureNodes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const int faceNumber       = faceNumbers[face];
      const int cell             = cellIndices[face];
      const int coordinateOffset = cell * DIMENSIONS;
      exahype2::dg::applyBoundaryConditions<Stub>(
        faceStates + faceIndices[face] * Setup::FaceStateCardinality,
        quadratureNodes,
        exahype2::dg::unittests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        faceNumber,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getBoundaryConditionsOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      faceStates
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::unittests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(geometry.getNumberOfWorkFaces()),
      ApplyBoundaryConditionsOfFace{
        deviceFaceStates.getData(),
        deviceFaceIndices.getData(),
        deviceFaceNumbers.getData(),
        deviceCellIndices.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceQuadratureNodes.getData()
      }
    );

    std::vector<double> result(faceStates.size());
    deviceFaceStates.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  struct SolveRiemannProblemOfFace final {
    const double*        faceStates{nullptr};
    double*              faceFluxes{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const std::uint8_t*  faceNumbers{nullptr};
    const std::uint64_t* cellIndices{nullptr};
    const double*        cellCentres{nullptr};
    const double*        cellSizes{nullptr};
    const double*        faceQuadratureWeights{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const int   faceNumber       = faceNumbers[face];
      const int   cell             = cellIndices[face];
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = faceStates + faceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = faceFluxes + faceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::solveRiemannProblem<Stub>(
        faceFlux,
        faceFlux + Setup::FaceSideFluxCardinality,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        faceQuadratureWeights,
        exahype2::dg::unittests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getRiemannFluxesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      faceStates,
    const std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::unittests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(geometry.getNumberOfWorkFaces()),
      SolveRiemannProblemOfFace{
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceFaceIndices.getData(),
        deviceFaceNumbers.getData(),
        deviceCellIndices.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceFaceQuadratureWeights.getData()
      }
    );

    std::vector<double> result(faceFluxes.size());
    deviceFaceFluxes.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  struct SolveLinearRiemannProblemOfFace final {
    const double*        faceStates{nullptr};
    double*              faceFluxes{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const std::uint8_t*  faceNumbers{nullptr};
    const std::uint64_t* cellIndices{nullptr};
    const double*        cellCentres{nullptr};
    const double*        cellSizes{nullptr};
    const double*        faceQuadratureWeights{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const int   faceNumber       = faceNumbers[face];
      const int   cell             = cellIndices[face];
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = faceStates + faceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = faceFluxes + faceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::solveLinearRiemannProblem<Stub>(
        faceFlux,
        faceFlux + Setup::FaceSideFluxCardinality,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        faceQuadratureWeights,
        exahype2::dg::unittests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getLinearRiemannFluxesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      faceStates,
    const std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::unittests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(geometry.getNumberOfWorkFaces()),
      SolveLinearRiemannProblemOfFace{
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceFaceIndices.getData(),
        deviceFaceNumbers.getData(),
        deviceCellIndices.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceFaceQuadratureWeights.getData()
      }
    );

    std::vector<double> result(faceFluxes.size());
    deviceFaceFluxes.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  struct AddFaceIntegralsOfCell final {
    double*              rightHandSideEstimates{nullptr};
    const double*        faceFluxes{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const double*        cellSizes{nullptr};
    const double*        faceQuadratureWeights{nullptr};
    const double*        inverseCellQuadratureWeights{nullptr};
    const double*        leftFaceCoefficients{nullptr};
    const double*        rightFaceCoefficients{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      const int   cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
      const int   coordinateOffset = cell * DIMENSIONS;
      auto* const cellUpdate       = rightHandSideEstimates + cellUpdateOffset;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        const double inverseCellSizeAlongNormal = 1.0 / cellSizes[coordinateOffset + direction];

        const auto* const lowerFaceFlux = faceFluxes
          + faceIndices[cell * peano4::FacesPerCell + direction] * Setup::FaceFluxCardinality
          + Setup::FaceSideFluxCardinality;
        const auto* const upperFaceFlux =
          faceFluxes + faceIndices[cell * peano4::FacesPerCell + direction + DIMENSIONS] * Setup::FaceFluxCardinality;
        for (int cellNode = 0; cellNode < Setup::NodesPerCell; cellNode++) {
          auto* const nodeUpdate = cellUpdate + static_cast<std::ptrdiff_t>(cellNode) * Setup::NumberOfUnknowns;
          exahype2::dg::addFaceIntegral<Stub>(
            nodeUpdate,
            cellNode,
            lowerFaceFlux,
            faceQuadratureWeights,
            inverseCellQuadratureWeights,
            leftFaceCoefficients,
            rightFaceCoefficients,
            direction,
            0,
            inverseCellSizeAlongNormal,
            Setup::TimeStepSize
          );
          exahype2::dg::addFaceIntegral<Stub>(
            nodeUpdate,
            cellNode,
            upperFaceFlux,
            faceQuadratureWeights,
            inverseCellQuadratureWeights,
            leftFaceCoefficients,
            rightFaceCoefficients,
            direction,
            1,
            inverseCellSizeAlongNormal,
            Setup::TimeStepSize
          );
        }
      }
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getFaceIntegralsOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      rightHandSideEstimates,
    const std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceRightHandSideEstimates(rightHandSideEstimates);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::unittests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceQuadratureWeights(operators.faceQuadratureWeights);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceInverseCellQuadratureWeights(
      operators.inverseCellQuadratureWeights
    );
    const exahype2::dg::unittests::DeviceBuffer<double> deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceRightFaceCoefficients(operators.rightFaceCoefficients);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      AddFaceIntegralsOfCell{
        deviceRightHandSideEstimates.getData(),
        deviceFaceFluxes.getData(),
        deviceFaceIndices.getData(),
        deviceCellSizes.getData(),
        deviceFaceQuadratureWeights.getData(),
        deviceInverseCellQuadratureWeights.getData(),
        deviceLeftFaceCoefficients.getData(),
        deviceRightFaceCoefficients.getData()
      }
    );

    std::vector<double> result(rightHandSideEstimates.size());
    deviceRightHandSideEstimates.copyBack(result);
    return result;
  }

  TARGET_DECLARE
  struct RaiseMaxEigenvaluesOfCell final {
    const double* cellStates{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    const double* quadratureNodes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              cell,
      exahype2::dg::MaxEigenvalueStatistics& maximum
    ) const {
      const int cellStateOffset  = cell * Setup::CellStateCardinality;
      const int coordinateOffset = cell * DIMENSIONS;
      maximum                    = exahype2::dg::MaxEigenvalueStatistics::combine(
        maximum,
        exahype2::dg::reduceMaxEigenvalues<Stub>(
          cellStates + cellStateOffset,
          quadratureNodes,
          tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
          tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
          Setup::TimeStamp,
          Setup::TimeStepSize,
          RuntimeParameters{}
        )
      );
    }
  };
  TARGET_DECLARE_END

  exahype2::dg::MaxEigenvalueStatistics getMaxEigenvaluesOf(
    const exahype2::dg::unittests::KernelOperators& operators,
    const exahype2::dg::unittests::CellChain&       geometry,
    const std::vector<double>&                      cellStates
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceQuadratureNodes(operators.quadratureNodes);

    exahype2::dg::MaxEigenvalueStatistics result = exahype2::dg::MaxEigenvalueStatistics::identity();
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(Setup::NumberOfCells),
      RaiseMaxEigenvaluesOfCell{
        deviceCellStates.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceQuadratureNodes.getData()
      },
      tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{result}
    );
    return result;
  }
} // namespace

UNIT_TEST("The projection clears the face buffers before it writes the state and the flux") {
  GIVEN("Face buffers filled with nine, a zero cell state, zero coefficients and quadrature nodes and a unit cell") {
    FaceBuffers buffers;
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
        buffers.state[face][entry] = 9.0;
      }
      for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
        buffers.flux[face][entry] = 9.0;
      }
    }

    double* state[peano4::FacesPerCell];
    double* flux[peano4::FacesPerCell];
    collectPointers(buffers, state, flux);

    const double                                cellState[CellNodes * NumberOfData] = {};
    const double                                coefficients[NodesPerAxis]          = {};
    const double                                quadratureNodes[NodesPerAxis]       = {};
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test extrapolates the state and the nodal fluxes of the flux solver to the faces") {
      exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients, coefficients);
      double nodalFluxes[DIMENSIONS * CellNodes * NumberOfUnknowns];
      exahype2::dg::evaluateNodalFluxes<FluxSolver>(
        nodalFluxes,
        cellState,
        quadratureNodes,
        coefficients,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );
      exahype2::dg::extrapolateFluxToFaces<FluxSolver>(flux, nodalFluxes, coefficients, coefficients);

      THEN("Every entry of every face buffer is zero") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
            VALIDATE_NUMERICAL_EQUALS(buffers.state[face][entry], 0.0);
          }
          for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
            VALIDATE_NUMERICAL_EQUALS(buffers.flux[face][entry], 0.0);
          }
        }
      }
    }
  }
}

UNIT_TEST("The extrapolation writes a constant state to every face bitwise") {
  GIVEN("A constant cell state without an exact binary representation and the basis values at both ends") {
    double cellState[CellNodes * NumberOfData] = {};
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.1 * value;
      }
    }

    const EndCoefficients coefficients;

    WHEN("The test extrapolates the state to the faces") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

      THEN("Every value of every face node equals its constant bitwise") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            for (int value = 0; value < NumberOfData; value++) {
              VALIDATE_EQUALS(
                state[face][faceNode * NumberOfData + value],
                1.0 / 3.0 + 0.1 * value,
                face,
                faceNode,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The extrapolation of each face node reads only the cell nodes of its own line") {
  GIVEN("Per cell node, a state that marks that node alone and rising left and falling right coefficients of sum one") {
    // The face coefficients of a Lagrange basis sum to one.
    constexpr double CoefficientSum = NodesPerAxis * (NodesPerAxis + 1.0) / 2.0;
    double           leftCoefficients[NodesPerAxis];
    double           rightCoefficients[NodesPerAxis];
    for (int index = 0; index < NodesPerAxis; index++) {
      leftCoefficients[index]  = (1.0 + index) / CoefficientSum;
      rightCoefficients[index] = (NodesPerAxis - index) / CoefficientSum;
    }

    double cellStates[CellNodes][CellNodes * NumberOfData] = {};
    for (int markedCellNode = 0; markedCellNode < CellNodes; markedCellNode++) {
      cellStates[markedCellNode][markedCellNode * NumberOfData] = 1.0;
    }

    WHEN("The test extrapolates the state of every marked node to the faces") {
      std::vector<FaceBuffers> buffersOfMarkedNode(CellNodes);
      for (int markedCellNode = 0; markedCellNode < CellNodes; markedCellNode++) {
        double* state[peano4::FacesPerCell];
        double* flux[peano4::FacesPerCell];
        collectPointers(buffersOfMarkedNode[markedCellNode], state, flux);
        exahype2::dg::extrapolateStateToFaces<Order2Layout>(
          state,
          cellStates[markedCellNode],
          leftCoefficients,
          rightCoefficients
        );
      }

      THEN("A lower face node equals the left coefficient of the marked node on its own line and zero elsewhere") {
        for (int markedCellNode = 0; markedCellNode < CellNodes; markedCellNode++) {
          for (int direction = 0; direction < DIMENSIONS; direction++) {
            for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
              double expected = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                if (getCellNodeOf(direction, faceNode, normalIndex) == markedCellNode) {
                  expected = leftCoefficients[normalIndex];
                }
              }
              const int faceEntry = faceNode * NumberOfData;
              VALIDATE_NUMERICAL_EQUALS(buffersOfMarkedNode[markedCellNode].state[direction][faceEntry], expected);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The two ends of every line use their own coefficients") {
  GIVEN("A state of slope two plus the axis number along every axis and the basis values at both ends") {
    double cellState[CellNodes * NumberOfData];
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const int cellNode = peano4::dLinearised(node, NodesPerAxis);
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 + 0.25 * value;
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        for (int value = 0; value < NumberOfData; value++) {
          cellState[cellNode * NumberOfData + value] += (2.0 + axis) * QuadratureNodes[node(axis)];
        }
      }
    });

    const EndCoefficients coefficients;

    WHEN("The test extrapolates the state to the faces") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

      THEN("The lower face contains the value of every line at zero and the upper face its value at one") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            const int firstCellNode = getCellNodeOf(direction, faceNode, 0);
            for (int value = 0; value < NumberOfData; value++) {
              const double expectedLower =
                cellState[firstCellNode * NumberOfData + value] - (2.0 + direction) * QuadratureNodes[0];
              VALIDATE_NUMERICAL_EQUALS(
                state[direction][faceNode * NumberOfData + value],
                expectedLower,
                direction,
                faceNode,
                value
              );
              VALIDATE_NUMERICAL_EQUALS(
                state[direction + DIMENSIONS][faceNode * NumberOfData + value],
                expectedLower + 2.0 + direction,
                direction,
                faceNode,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The extrapolation writes a constant float state unchanged to every face node of double") {
  GIVEN("A constant float state without an exact binary representation and the basis values at both ends") {
    float cellState[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = static_cast<float>(1.0 / 3.0 + 0.1 * value);
      }
    }

    const EndCoefficients coefficients;

    WHEN("The test extrapolates the float state to faces of double") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

      THEN("Every value of every face node equals the float value of its constant bitwise") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            for (int value = 0; value < NumberOfData; value++) {
              VALIDATE_EQUALS(
                state[face][faceNode * NumberOfData + value],
                static_cast<double>(static_cast<float>(1.0 / 3.0 + 0.1 * value)),
                face,
                faceNode,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The projection evaluates the flux at the cell nodes before the extrapolation") {
  GIVEN("A state that differs per node and unknown, the left basis values as coefficients and a unit cell") {
    const EndCoefficients endCoefficients;
    const double* const   coefficients = endCoefficients.left;

    double cellState[CellNodes * NumberOfData] = {};
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        cellState[cellNode * NumberOfData + unknown] = 1.0 + cellNode + 0.25 * unknown;
      }
    }

    const double                                quadratureNodes[NodesPerAxis] = {};
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The flux solver evaluates its nodal fluxes and extrapolates them to the faces") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      double nodalFluxes[DIMENSIONS * CellNodes * NumberOfUnknowns];
      exahype2::dg::evaluateNodalFluxes<FluxSolver>(
        nodalFluxes,
        cellState,
        quadratureNodes,
        coefficients,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );
      exahype2::dg::extrapolateFluxToFaces<FluxSolver>(flux, nodalFluxes, coefficients, coefficients);

      THEN("Both faces of every direction contain the extrapolation of the solver flux at the cell nodes") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              double expected = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                const int cellNode = getCellNodeOf(direction, faceNode, normalIndex);
                expected +=
                  (direction + 1.0) * cellState[cellNode * NumberOfData + unknown] * coefficients[normalIndex];
              }
              VALIDATE_NUMERICAL_EQUALS(flux[direction][faceNode * NumberOfUnknowns + unknown], expected);
              VALIDATE_NUMERICAL_EQUALS(flux[direction + DIMENSIONS][faceNode * NumberOfUnknowns + unknown], expected);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A system without flux leaves zeros on every face") {
  GIVEN("A zero cell state, the coefficients one to three, zero quadrature nodes and a unit cell") {
    const double cellState[CellNodes * NumberOfData] = {};
    double       coefficients[NodesPerAxis];
    const double quadratureNodes[NodesPerAxis] = {};
    for (int index = 0; index < NodesPerAxis; index++) {
      coefficients[index] = 1.0 + index;
    }
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("A solver without flux evaluates its nodal fluxes and extrapolates them to the faces") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      double nodalFluxes[DIMENSIONS * CellNodes * NumberOfUnknowns];
      exahype2::dg::evaluateNodalFluxes<Order2Layout>(
        nodalFluxes,
        cellState,
        quadratureNodes,
        coefficients,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );
      exahype2::dg::extrapolateFluxToFaces<Order2Layout>(flux, nodalFluxes, coefficients, coefficients);

      THEN("Every flux entry of every face is zero") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
            VALIDATE_NUMERICAL_EQUALS(flux[face][entry], 0.0);
          }
        }
      }
    }
  }
}

UNIT_TEST("The projection extrapolates a diffusive flux with the nodal gradients") {
  GIVEN("A state in a cell whose extent differs per axis, its gradients along every line and the left basis values") {
    const DerivativeOperator                    derivativeOperator;
    const EndCoefficients                       endCoefficients;
    const double* const                         coefficients                  = endCoefficients.left;
    const double                                quadratureNodes[NodesPerAxis] = {0.1, 0.5, 0.9};
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
    tarch::la::Vector<DIMENSIONS, double>       cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }

    double cellState[CellNodes * NumberOfData];
    for (int entry = 0; entry < CellNodes * NumberOfData; entry++) {
      cellState[entry] = 1.0 + 0.5 * entry - 0.01 * entry * entry;
    }

    double gradients[DIMENSIONS][CellNodes * NumberOfData] = {};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int line = 0; line < FaceNodes; line++) {
        for (int targetIndex = 0; targetIndex < NodesPerAxis; targetIndex++) {
          const int targetNode = getCellNodeOf(axis, line, targetIndex);
          for (int sourceIndex = 0; sourceIndex < NodesPerAxis; sourceIndex++) {
            const int    sourceNode = getCellNodeOf(axis, line, sourceIndex);
            const double weight = derivativeOperator.entries[targetIndex * NodesPerAxis + sourceIndex] / cellSize(axis);
            for (int value = 0; value < NumberOfData; value++) {
              gradients[axis][targetNode * NumberOfData + value] +=
                weight * cellState[sourceNode * NumberOfData + value];
            }
          }
        }
      }
    }

    WHEN("The diffusive flux solver extrapolates its flux to the faces") {
      FaceBuffers buffers;
      double*     state[peano4::FacesPerCell];
      double*     flux[peano4::FacesPerCell];
      collectPointers(buffers, state, flux);
      double nodalFluxes[DIMENSIONS * CellNodes * NumberOfUnknowns];
      exahype2::dg::evaluateNodalFluxes<DiffusiveFluxSolver>(
        nodalFluxes,
        cellState,
        quadratureNodes,
        derivativeOperator.entries,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );
      exahype2::dg::extrapolateFluxToFaces<DiffusiveFluxSolver>(flux, nodalFluxes, coefficients, coefficients);

      THEN("Both faces of every direction contain the extrapolation of the flux of the gradients at the cell nodes") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              double expected = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                const int cellNode = getCellNodeOf(direction, faceNode, normalIndex);
                double    nodeFlux = 0.0;
                for (int axis = 0; axis < DIMENSIONS; axis++) {
                  nodeFlux += (1.0 + axis) * gradients[axis][cellNode * NumberOfData + unknown];
                }
                expected += nodeFlux * coefficients[normalIndex];
              }
              VALIDATE_NUMERICAL_EQUALS(
                flux[direction][faceNode * NumberOfUnknowns + unknown],
                expected,
                direction,
                faceNode,
                unknown
              );
              VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("The boundary conditions fill the outer half of a boundary face from the inner half") {
  GIVEN("Per face of a cell, a face state with a ramp in the half the cell writes and minus one in the outer half") {
    double faceStates[peano4::FacesPerCell][2 * FaceNodes * NumberOfData];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      fillInnerHalf(faceStates[faceNumber], getInnerSideOf(faceNumber));
    }

    WHEN("The recording solver applies its boundary conditions to every face at the time stamp 0.75") {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        exahype2::dg::applyBoundaryConditions<RecordingSolver>(
          faceStates[faceNumber],
          BoundaryQuadratureNodes,
          getFaceCentre(faceNumber),
          getCellSize(),
          BoundaryTimeStamp,
          faceNumber,
          RuntimeParameters{}
        );
      }

      THEN("The inner half keeps its ramp and the outer half contains the state the solver records from it") {
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const int innerSide = getInnerSideOf(faceNumber);
          const int outerSide = 1 - innerSide;
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            const auto position = getNodePositionOf(faceNumber, faceNode);
            for (int value = 0; value < NumberOfData; value++) {
              const int    innerEntry = (innerSide * FaceNodes + faceNode) * NumberOfData + value;
              const int    outerEntry = (outerSide * FaceNodes + faceNode) * NumberOfData + value;
              const double expected   = 2.0 * faceStates[faceNumber][innerEntry] + position(faceNumber % DIMENSIONS)
                + 10.0 * position((faceNumber % DIMENSIONS + 1) % DIMENSIONS) + BoundaryTimeStamp;
              VALIDATE_NUMERICAL_EQUALS(
                faceStates[faceNumber][innerEntry],
                0.25 * (faceNode * NumberOfData + value),
                faceNumber,
                faceNode
              );
              VALIDATE_NUMERICAL_EQUALS(faceStates[faceNumber][outerEntry], expected, faceNumber, faceNode);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The boundary conditions receive the position of every node of a boundary face") {
  GIVEN("Per face of a cell, a zero face state") {
    double faceStates[peano4::FacesPerCell][2 * FaceNodes * NumberOfData] = {};

    WHEN("The recording solver applies its boundary conditions to every face at the time stamp zero") {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        exahype2::dg::applyBoundaryConditions<RecordingSolver>(
          faceStates[faceNumber],
          BoundaryQuadratureNodes,
          getFaceCentre(faceNumber),
          getCellSize(),
          0.0,
          faceNumber,
          RuntimeParameters{}
        );
      }

      THEN("The outer half records every node position and each normal coordinate equals that of the face centre") {
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const int outerSide = 1 - getInnerSideOf(faceNumber);
          const int direction = faceNumber % DIMENSIONS;
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            const auto   position   = getNodePositionOf(faceNumber, faceNode);
            const int    outerEntry = (outerSide * FaceNodes + faceNode) * NumberOfData;
            const double recorded   = faceStates[faceNumber][outerEntry];
            VALIDATE_NUMERICAL_EQUALS(
              recorded,
              position(direction) + 10.0 * position((direction + 1) % DIMENSIONS),
              faceNumber,
              faceNode
            );
            VALIDATE_NUMERICAL_EQUALS(position(direction), getFaceCentre(faceNumber)(direction), faceNumber, faceNode);
          }
        }
      }
    }
  }
}

UNIT_TEST("The flux on a face reads the face state of the selected side") {
  GIVEN("Face states that differ per entry on both sides and, per direction and side, a flux filled with nine") {
    const double                                quadratureNodes[NodesPerAxis] = {0.1, 0.5, 0.9};
    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    double leftState[FaceNodes * NumberOfData];
    double rightState[FaceNodes * NumberOfData];
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      leftState[entry]  = 1.0 + 0.5 * entry;
      rightState[entry] = -2.0 + 0.25 * entry;
    }

    double faceFluxes[DIMENSIONS][2][FaceNodes * NumberOfUnknowns];
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int side = 0; side < 2; side++) {
        for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
          faceFluxes[direction][side][entry] = 9.0;
        }
      }
    }

    WHEN("The flux solver evaluates the flux of either side on the face of every direction") {
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int side = 0; side < 2; side++) {
          exahype2::dg::evaluateFluxOnFace<FluxSolver>(
            faceFluxes[direction][side],
            leftState,
            rightState,
            side,
            quadratureNodes,
            faceCentre,
            cellSize,
            0.0,
            0.0,
            direction,
            RuntimeParameters{}
          );
        }
      }

      THEN("Every node equals the multiple of the state of the selected side that the solver adds to a cleared flux") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int side = 0; side < 2; side++) {
            const double* const faceState = side == 0 ? leftState : rightState;
            for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
              for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
                VALIDATE_NUMERICAL_EQUALS(
                  faceFluxes[direction][side][faceNode * NumberOfUnknowns + unknown],
                  (direction + 1.0) * faceState[faceNode * NumberOfData + unknown],
                  direction,
                  side,
                  faceNode
                );
              }
            }
          }
        }
      }

      AND_WHEN("A solver without flux evaluates the flux of either side into the same buffers") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int side = 0; side < 2; side++) {
            exahype2::dg::evaluateFluxOnFace<Order2Layout>(
              faceFluxes[direction][side],
              leftState,
              rightState,
              side,
              quadratureNodes,
              faceCentre,
              cellSize,
              0.0,
              0.0,
              direction,
              RuntimeParameters{}
            );
          }
        }

        THEN("Every entry of every buffer is zero") {
          for (int direction = 0; direction < DIMENSIONS; direction++) {
            for (int side = 0; side < 2; side++) {
              for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
                VALIDATE_NUMERICAL_EQUALS(faceFluxes[direction][side][entry], 0.0);
              }
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A diffusive flux on a face reads the jump along the normal") {
  GIVEN("Face states that differ per entry on both sides and a cell whose extent differs per axis") {
    const double                                quadratureNodes[NodesPerAxis] = {0.1, 0.5, 0.9};
    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    tarch::la::Vector<DIMENSIONS, double>       cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }

    double leftState[FaceNodes * NumberOfData];
    double rightState[FaceNodes * NumberOfData];
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      leftState[entry]  = 1.0 + 0.5 * entry;
      rightState[entry] = 4.0 - 0.25 * entry;
    }

    WHEN("The diffusive flux solver evaluates the flux of either side on the face of every direction") {
      double faceFluxes[DIMENSIONS][2][FaceNodes * NumberOfUnknowns] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int side = 0; side < 2; side++) {
          exahype2::dg::evaluateFluxOnFace<DiffusiveFluxSolver>(
            faceFluxes[direction][side],
            leftState,
            rightState,
            side,
            quadratureNodes,
            faceCentre,
            cellSize,
            0.0,
            0.0,
            direction,
            RuntimeParameters{}
          );
        }
      }

      THEN("Both sides equal the multiple of the state jump over the cell extent along the normal") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int side = 0; side < 2; side++) {
            for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
              for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
                const int    entry = faceNode * NumberOfData + unknown;
                const double expected =
                  (direction + 1.0) * (rightState[entry] - leftState[entry]) / cellSize(direction);
                VALIDATE_NUMERICAL_EQUALS(
                  faceFluxes[direction][side][faceNode * NumberOfUnknowns + unknown],
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
    }
  }
}

UNIT_TEST("With two equal states the Riemann solver averages the two fluxes and each side keeps half the flux jump") {
  GIVEN("The state two on both sides, the own fluxes one and three plus the unknown index and normalised weights") {
    constexpr int Unknowns = EigenvalueSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + EigenvalueSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns];
    double rightFlux[RiemannFaceNodes * Unknowns];
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        leftFlux[faceNode * Unknowns + unknown]  = 1.0 + unknown;
        rightFlux[faceNode * Unknowns + unknown] = 3.0 + unknown;
      }
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 2.0;
        rightState[faceNode * Data + value] = 2.0;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem along the first axis") {
      exahype2::dg::solveRiemannProblem<EigenvalueSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("Both sides equal one, half the flux jump") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          for (int unknown = 0; unknown < Unknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(leftFlux[faceNode * Unknowns + unknown], 1.0);
            VALIDATE_NUMERICAL_EQUALS(rightFlux[faceNode * Unknowns + unknown], 1.0);
          }
        }
      }
    }
  }
}

UNIT_TEST("Two sides with equal states and fluxes leave zeros bitwise with a nonconservative product") {
  GIVEN("Equal states and own fluxes on both sides without an exact binary representation and normalised weights") {
    constexpr int Unknowns = ProductSolverWithAuxiliary::NumberOfUnknowns;
    constexpr int Data     = Unknowns + ProductSolverWithAuxiliary::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns];
    double rightFlux[RiemannFaceNodes * Unknowns];
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        leftFlux[faceNode * Unknowns + unknown]  = 1.0 / 3.0 + 0.1 * unknown;
        rightFlux[faceNode * Unknowns + unknown] = 1.0 / 3.0 + 0.1 * unknown;
      }
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 2.0 / 3.0 + 0.1 * value;
        rightState[faceNode * Data + value] = 2.0 / 3.0 + 0.1 * value;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem of the product solver with an auxiliary variable along the first axis") {
      exahype2::dg::solveRiemannProblem<ProductSolverWithAuxiliary>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("Every flux entry of both sides is zero") {
        for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
          VALIDATE_EQUALS(leftFlux[entry], 0.0, entry);
          VALIDATE_EQUALS(rightFlux[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("The jump term scales with the larger wave speed of the two sides") {
  GIVEN("Zero own fluxes, the left states three and one, the right states five and four and normalised weights") {
    constexpr int Unknowns = EigenvalueSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + EigenvalueSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns]  = {};
    double rightFlux[RiemannFaceNodes * Unknowns] = {};
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      const int firstEntry       = faceNode * Data;
      leftState[firstEntry]      = 3.0;
      leftState[firstEntry + 1]  = 1.0;
      rightState[firstEntry]     = 5.0;
      rightState[firstEntry + 1] = 4.0;
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem along the first axis") {
      exahype2::dg::solveRiemannProblem<EigenvalueSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The left side is half the larger wave speed five times the state jump and the right side its negative") {
        constexpr double ExpectedMaxEigenvalue = 5.0;
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          const int firstEntry = faceNode * Unknowns;
          VALIDATE_NUMERICAL_EQUALS(leftFlux[firstEntry], 0.5 * ExpectedMaxEigenvalue * (3.0 - 5.0));
          VALIDATE_NUMERICAL_EQUALS(leftFlux[firstEntry + 1], 0.5 * ExpectedMaxEigenvalue * (1.0 - 4.0));
          VALIDATE_NUMERICAL_EQUALS(rightFlux[firstEntry], -0.5 * ExpectedMaxEigenvalue * (3.0 - 5.0));
          VALIDATE_NUMERICAL_EQUALS(rightFlux[firstEntry + 1], -0.5 * ExpectedMaxEigenvalue * (1.0 - 4.0));
        }
      }
    }
  }
}

UNIT_TEST("Without a nonconservative product the two stored differences add up to the jump of the own fluxes") {
  GIVEN("Own fluxes and states that differ per node on both sides, an auxiliary variable and normalised weights") {
    constexpr int Unknowns = EigenvalueSolverWithAuxiliary::NumberOfUnknowns;
    constexpr int Data     = Unknowns + EigenvalueSolverWithAuxiliary::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns];
    double rightFlux[RiemannFaceNodes * Unknowns];
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        leftFlux[faceNode * Unknowns + unknown]  = 0.5 * faceNode + unknown;
        rightFlux[faceNode * Unknowns + unknown] = 1.5 * faceNode - unknown;
      }
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 1.0 + 0.25 * value;
        rightState[faceNode * Data + value] = 2.0 - 0.25 * value;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem along the first axis") {
      exahype2::dg::solveRiemannProblem<EigenvalueSolverWithAuxiliary>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The two differences of every entry add up to the right own flux minus the left own flux") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          for (int unknown = 0; unknown < Unknowns; unknown++) {
            const int entry = faceNode * Unknowns + unknown;
            VALIDATE_NUMERICAL_EQUALS(
              leftFlux[entry] + rightFlux[entry],
              (1.5 * faceNode - unknown) - (0.5 * faceNode + unknown),
              entry
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The nonconservative jump term adds to both stored differences with the same sign") {
  GIVEN("Zero own fluxes, the state one on the left, one plus the value index on the right and normalised weights") {
    constexpr int Unknowns = ProductSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + ProductSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns]  = {};
    double rightFlux[RiemannFaceNodes * Unknowns] = {};
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 1.0;
        rightState[faceNode * Data + value] = 1.0 + value;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem along the first axis for a product that returns the state jump") {
      exahype2::dg::solveRiemannProblem<ProductSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The two differences of every unknown add up to its state jump") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          for (int unknown = 0; unknown < Unknowns; unknown++) {
            const double stateJump = rightState[faceNode * Data + unknown] - leftState[faceNode * Data + unknown];
            VALIDATE_NUMERICAL_EQUALS(
              leftFlux[faceNode * Unknowns + unknown] + rightFlux[faceNode * Unknowns + unknown],
              stateJump
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The nonconservative product reads the jump of every auxiliary variable next to the unknowns") {
  GIVEN("Zero own fluxes, unknowns without a jump, an auxiliary variable that jumps per node and normalised weights") {
    constexpr int Unknowns = AuxiliaryJumpSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + AuxiliaryJumpSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns]  = {};
    double rightFlux[RiemannFaceNodes * Unknowns] = {};
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      const int firstEntry = faceNode * Data;
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        leftState[firstEntry + unknown]  = 1.0;
        rightState[firstEntry + unknown] = 1.0;
      }
      leftState[firstEntry + Unknowns]  = 2.0 + faceNode;
      rightState[firstEntry + Unknowns] = 5.0 + 2.0 * faceNode;
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the Riemann problem along the first axis for a product that reads the auxiliary jump") {
      exahype2::dg::solveRiemannProblem<AuxiliaryJumpSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("Both sides of every unknown equal half of one plus the unknown index times the auxiliary jump") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          const double auxiliaryJump = rightState[faceNode * Data + Unknowns] - leftState[faceNode * Data + Unknowns];
          for (int unknown = 0; unknown < Unknowns; unknown++) {
            const double halfProduct = 0.5 * (1.0 + unknown) * auxiliaryJump;
            VALIDATE_NUMERICAL_EQUALS(leftFlux[faceNode * Unknowns + unknown], halfProduct, faceNode, unknown);
            VALIDATE_NUMERICAL_EQUALS(rightFlux[faceNode * Unknowns + unknown], halfProduct, faceNode, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("The linear form builds the flux from the state jump") {
  GIVEN("Own fluxes nine and minus nine, the left state one, the right state three and normalised weights") {
    constexpr int Unknowns = LinearFluxSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + LinearFluxSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns];
    double rightFlux[RiemannFaceNodes * Unknowns];
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
      leftFlux[entry]  = 9.0;
      rightFlux[entry] = -9.0;
    }
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 1.0;
        rightState[faceNode * Data + value] = 3.0;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the linear Riemann problem of the solver whose flux returns its state along the first axis") {
      exahype2::dg::solveLinearRiemannProblem<LinearFluxSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The central flux is one and the dissipation of wave speed three adds to the sides with opposite signs") {
        constexpr double ExpectedFlux          = 1.0;
        constexpr double ExpectedMaxEigenvalue = 3.0;
        constexpr double ExpectedHalfStateJump = 1.0;
        for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
          VALIDATE_NUMERICAL_EQUALS(leftFlux[entry], ExpectedFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
          VALIDATE_NUMERICAL_EQUALS(rightFlux[entry], ExpectedFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
        }
      }
    }
  }
}

UNIT_TEST("The linear form adds the nonconservative product") {
  GIVEN("Zero own fluxes, the left state one, the right state three and normalised weights") {
    constexpr int Unknowns = LinearFluxProductSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + LinearFluxProductSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns]  = {};
    double rightFlux[RiemannFaceNodes * Unknowns] = {};
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 1.0;
        rightState[faceNode * Data + value] = 3.0;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the linear Riemann problem of the solver with a flux and a product of the state jump") {
      exahype2::dg::solveLinearRiemannProblem<LinearFluxProductSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The central flux doubles to two and the dissipation of wave speed three keeps its opposite signs") {
        constexpr double ExpectedCentralFlux   = 2.0;
        constexpr double ExpectedMaxEigenvalue = 3.0;
        constexpr double ExpectedHalfStateJump = 1.0;
        for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
          VALIDATE_NUMERICAL_EQUALS(
            leftFlux[entry],
            ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump
          );
          VALIDATE_NUMERICAL_EQUALS(
            rightFlux[entry],
            ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump
          );
        }
      }
    }
  }
}

UNIT_TEST("The linear form applies the material parameter matrix") {
  GIVEN("Zero own fluxes, the left state one, the right state three and normalised weights") {
    constexpr int Unknowns = LinearFluxMaterialSolver::NumberOfUnknowns;
    constexpr int Data     = Unknowns + LinearFluxMaterialSolver::NumberOfAuxiliaryVariables;

    double faceQuadratureWeights[RiemannFaceNodes];
    fillNormalisedWeights(faceQuadratureWeights);

    double leftFlux[RiemannFaceNodes * Unknowns]  = {};
    double rightFlux[RiemannFaceNodes * Unknowns] = {};
    double leftState[RiemannFaceNodes * Data];
    double rightState[RiemannFaceNodes * Data];
    for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
      for (int value = 0; value < Data; value++) {
        leftState[faceNode * Data + value]  = 1.0;
        rightState[faceNode * Data + value] = 3.0;
      }
    }

    const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The test solves the linear Riemann problem of the solver whose material matrix doubles the flux") {
      exahype2::dg::solveLinearRiemannProblem<LinearFluxMaterialSolver>(
        leftFlux,
        rightFlux,
        leftState,
        rightState,
        faceQuadratureWeights,
        faceCentre,
        cellSize,
        0.0,
        1.0,
        0,
        RuntimeParameters{}
      );

      THEN("The material matrix doubles the central flux to two and the dissipation keeps its opposite signs") {
        constexpr double ExpectedCentralFlux   = 2.0;
        constexpr double ExpectedMaxEigenvalue = 3.0;
        constexpr double ExpectedHalfStateJump = 1.0;
        for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
          VALIDATE_NUMERICAL_EQUALS(
            leftFlux[entry],
            ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump
          );
          VALIDATE_NUMERICAL_EQUALS(
            rightFlux[entry],
            ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump
          );
        }
      }
    }
  }
}

UNIT_TEST("An interior face stores the differences between the Rusanov flux and the own fluxes of its sides") {
  GIVEN("A face with rising states and own fluxes below and the state one and the own flux twenty above") {
    Face face;
    fill(face);

    WHEN("The test solves the Riemann problem of the interior face along the first axis") {
      solveFace<FaceSolver>(face, 0, -1);

      THEN("The lower half contains the Rusanov flux minus the own flux and the upper half the reverse difference") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          const double lowerOwnFlux  = LowerOwnFlux + faceNode;
          const double numericalFlux = 0.5 * (lowerOwnFlux + UpperOwnFlux + WaveSpeed * ((2.0 + faceNode) - 1.0));
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[faceNode], numericalFlux - lowerOwnFlux, faceNode);
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[FaceFluxSize + faceNode], UpperOwnFlux - numericalFlux, faceNode);
        }
      }
    }
  }
}

UNIT_TEST("A boundary face takes the boundary state into its outer half and the flux difference into its inner half") {
  GIVEN("A face with rising states and own fluxes below and the state one and the own flux twenty above") {
    Face face;
    fill(face);

    WHEN("A solver without flux solves the face as the lower boundary face of the cell inside") {
      solveFace<FaceSolver>(face, 0, 0);

      THEN("The outer half contains the outside state and the inner half keeps its state") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          VALIDATE_NUMERICAL_EQUALS(face.faceState[faceNode], OutsideValue, faceNode);
          VALIDATE_NUMERICAL_EQUALS(face.faceState[FaceStateSize + faceNode], 1.0, faceNode);
        }
      }

      THEN("The inner half contains its own flux minus the Rusanov flux") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          const double numericalFlux = 0.5 * (0.0 + UpperOwnFlux + WaveSpeed * (OutsideValue - 1.0));
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[FaceFluxSize + faceNode], UpperOwnFlux - numericalFlux, faceNode);
        }
      }
    }
  }
}

UNIT_TEST("A solver's own Riemann solver receives the time stamp, step size, normal, boundary flag and face index") {
  GIVEN("A face with rising states and own fluxes below and the state one and the own flux twenty above") {
    Face face;
    fill(face);

    WHEN("The solver with its own Riemann solver solves the face as the upper boundary face of the first axis") {
      constexpr int UpperFaceOfTheFirstAxis = DIMENSIONS;
      solveFace<FaceSolverWithRiemannSolver>(face, 0, UpperFaceOfTheFirstAxis);

      THEN("The lower half contains the encoded arguments of one call and the upper half their negative") {
        const double increment = encodeFaceArguments(TimeStamp, TimeStepSize, 0, true, UpperFaceOfTheFirstAxis);
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[faceNode], increment, faceNode);
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[FaceFluxSize + faceNode], -increment, faceNode);
        }
      }
    }
  }
}

UNIT_TEST("The linear Riemann solver of a system without flux writes opposite dissipations into the two flux halves") {
  GIVEN("A face with rising states and own fluxes below and the state one and the own flux twenty above") {
    Face face;
    fill(face);

    WHEN("The test solves the linear Riemann problem of the interior face along the first axis") {
      solveFace<LinearFaceSolver>(face, 0, -1);

      THEN("The lower half contains minus the dissipation of the state jump and the upper half the dissipation") {
        for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
          const double dissipation = WaveSpeed * 0.5 * (1.0 - (2.0 + faceNode));
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[faceNode], -dissipation, faceNode);
          VALIDATE_NUMERICAL_EQUALS(face.faceFlux[FaceFluxSize + faceNode], dissipation, faceNode);
        }
      }
    }
  }
}

UNIT_TEST("A zero flux leaves the update untouched") {
  GIVEN("Face tables, a zero face flux and, per direction and orientation, an update counting up from one third") {
    const Tables tables                                 = getTables();
    const double faceFlux[FaceNodes * NumberOfUnknowns] = {};

    double valuesBeforeTheIntegral[CellNodes * NumberOfUnknowns];
    for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
      valuesBeforeTheIntegral[entry] = 1.0 / 3.0 + entry;
    }
    double cellUpdates[DIMENSIONS][2][CellNodes * NumberOfUnknowns];
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int orientation = 0; orientation < 2; orientation++) {
        for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
          cellUpdates[direction][orientation][entry] = valuesBeforeTheIntegral[entry];
        }
      }
    }

    WHEN("The integral of the zero flux adds to the update of every direction and orientation") {
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int orientation = 0; orientation < 2; orientation++) {
          addFaceIntegralToEveryNode(
            cellUpdates[direction][orientation],
            faceFlux,
            tables,
            direction,
            orientation,
            2.0,
            0.5
          );
        }
      }

      THEN("Every update keeps its values bitwise") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int orientation = 0; orientation < 2; orientation++) {
            for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
              VALIDATE_EQUALS(
                cellUpdates[direction][orientation][entry],
                valuesBeforeTheIntegral[entry],
                direction,
                orientation,
                entry
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Each cell node receives the flux of its own face node") {
  GIVEN("Face tables with unit inverse cell weights and, per face node, a flux that marks that node alone") {
    Tables tables = getTables();
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      tables.inverseCellQuadratureWeights[cellNode] = 1.0;
    }

    double faceFluxes[FaceNodes][FaceNodes * NumberOfUnknowns] = {};
    for (int markedFaceNode = 0; markedFaceNode < FaceNodes; markedFaceNode++) {
      faceFluxes[markedFaceNode][markedFaceNode * NumberOfUnknowns] = 1.0;
    }

    WHEN("The integral of every marked flux over the lower face of every direction adds to a zero update") {
      double cellUpdates[DIMENSIONS][FaceNodes][CellNodes * NumberOfUnknowns] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int markedFaceNode = 0; markedFaceNode < FaceNodes; markedFaceNode++) {
          addFaceIntegralToEveryNode(
            cellUpdates[direction][markedFaceNode],
            faceFluxes[markedFaceNode],
            tables,
            direction,
            0,
            1.0,
            1.0
          );
        }
      }

      THEN("The nodes on the line of the marked node receive its weighted flux and every other entry stays zero") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int markedFaceNode = 0; markedFaceNode < FaceNodes; markedFaceNode++) {
            const double* const cellUpdate  = cellUpdates[direction][markedFaceNode];
            double              expectedSum = 0.0;
            for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
              const int    cellNode   = getCellNodeOf(direction, markedFaceNode, normalIndex);
              const int    firstEntry = cellNode * NumberOfUnknowns;
              const double expectedValue =
                -tables.faceQuadratureWeights[markedFaceNode] * tables.leftFaceCoefficients[normalIndex];
              VALIDATE_NUMERICAL_EQUALS(cellUpdate[firstEntry], expectedValue);
              VALIDATE_NUMERICAL_EQUALS(cellUpdate[firstEntry + 1], 0.0);
              expectedSum += expectedValue;
            }

            double sumOfUpdates = 0.0;
            for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
              sumOfUpdates += cellUpdate[entry];
            }
            VALIDATE_NUMERICAL_EQUALS(sumOfUpdates, expectedSum);
          }
        }
      }
    }
  }
}

UNIT_TEST("The update strides by the unknowns and keeps the entries behind the last cell node") {
  GIVEN("Face tables, a face flux that differs per entry and a zero update followed by guard entries of seven") {
    constexpr int    Direction    = DIMENSIONS - 1;
    constexpr int    Orientation  = 1;
    constexpr int    GuardEntries = 4;
    constexpr double GuardValue   = 7.0;

    const Tables tables = getTables();
    double       faceFlux[FaceNodes * NumberOfUnknowns];
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      faceFlux[entry] = 1.0 + entry;
    }

    double cellUpdate[CellNodes * NumberOfUnknowns + GuardEntries] = {};
    for (int entry = 0; entry < GuardEntries; entry++) {
      cellUpdate[CellNodes * NumberOfUnknowns + entry] = GuardValue;
    }

    WHEN("The integral over the upper face of the last axis adds to the update") {
      addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, Direction, Orientation, 1.0, 1.0);

      THEN("Every unknown of every node receives its own weighted face value and the guard entries keep seven") {
        for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
          for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
            const int cellNode = getCellNodeOf(Direction, faceNode, normalIndex);
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode]
                * tables.faceQuadratureWeights[faceNode] * tables.rightFaceCoefficients[normalIndex]
                * faceFlux[faceNode * NumberOfUnknowns + unknown];
              VALIDATE_NUMERICAL_EQUALS(
                cellUpdate[cellNode * NumberOfUnknowns + unknown],
                expectedValue,
                cellNode,
                unknown
              );
            }
          }
        }

        for (int entry = 0; entry < GuardEntries; entry++) {
          VALIDATE_NUMERICAL_EQUALS(cellUpdate[CellNodes * NumberOfUnknowns + entry], GuardValue, entry);
        }
      }
    }
  }
}

UNIT_TEST("The lower and the upper face subtract their flux differences with the same sign") {
  GIVEN("Face tables with equal coefficients at both ends and a face flux that differs per entry") {
    Tables tables = getTables();
    for (int index = 0; index < NodesPerAxis; index++) {
      tables.rightFaceCoefficients[index] = tables.leftFaceCoefficients[index];
    }

    double faceFlux[FaceNodes * NumberOfUnknowns];
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      faceFlux[entry] = 1.0 + entry;
    }

    WHEN("The integral of every direction adds to one zero update as a lower face and to another as an upper face") {
      double lowerFaceUpdates[DIMENSIONS][CellNodes * NumberOfUnknowns] = {};
      double upperFaceUpdates[DIMENSIONS][CellNodes * NumberOfUnknowns] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int orientation = 0; orientation < 2; orientation++) {
          double* const update = (orientation == 0) ? lowerFaceUpdates[direction] : upperFaceUpdates[direction];
          addFaceIntegralToEveryNode(update, faceFlux, tables, direction, orientation, 1.0, 1.0);
        }
      }

      THEN("Both updates agree bitwise") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
            VALIDATE_EQUALS(lowerFaceUpdates[direction][entry], upperFaceUpdates[direction][entry], direction, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("The face integral multiplies the weights and the coefficients") {
  GIVEN("Face tables, a face flux that differs per entry, the inverse cell size three and the step size 0.25") {
    constexpr int    Direction       = 0;
    constexpr int    Orientation     = 1;
    constexpr double InverseCellSize = 3.0;
    constexpr double StepSize        = 0.25;

    const Tables tables = getTables();
    double       faceFlux[FaceNodes * NumberOfUnknowns];
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      faceFlux[entry] = 0.5 + 0.25 * entry;
    }

    WHEN("The integral over the upper face of the first axis adds to a zero update") {
      double cellUpdate[CellNodes * NumberOfUnknowns] = {};
      addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, Direction, Orientation, InverseCellSize, StepSize);

      THEN("Every entry is minus the inverse cell weight times the scaled face weight, coefficient and flux") {
        for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
          const double scaledWeight = InverseCellSize * StepSize * tables.faceQuadratureWeights[faceNode];
          for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
            const int cellNode = getCellNodeOf(Direction, faceNode, normalIndex);
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode] * scaledWeight
                * tables.rightFaceCoefficients[normalIndex] * faceFlux[faceNode * NumberOfUnknowns + unknown];
              VALIDATE_NUMERICAL_EQUALS(cellUpdate[cellNode * NumberOfUnknowns + unknown], expectedValue);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The face integral of one node writes the updates of its unknowns and keeps the entries behind them") {
  GIVEN("Face tables, a face flux that differs per entry and one node update followed by guard entries of seven") {
    constexpr int    Direction    = DIMENSIONS - 1;
    constexpr int    Orientation  = 0;
    constexpr int    CellNode     = CellNodes - 2;
    constexpr double GuardValue   = 7.0;
    constexpr int    GuardEntries = 2;

    const Tables tables = getTables();
    double       faceFlux[FaceNodes * NumberOfUnknowns];
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      faceFlux[entry] = 1.0 + entry;
    }

    double nodeUpdate[NumberOfUnknowns + GuardEntries] = {};
    for (int entry = NumberOfUnknowns; entry < NumberOfUnknowns + GuardEntries; entry++) {
      nodeUpdate[entry] = GuardValue;
    }

    WHEN("The lower face integral of the last axis adds to the node update and to a zero update of every node") {
      double everyNodeUpdate[CellNodes * NumberOfUnknowns] = {};
      addFaceIntegralToEveryNode(everyNodeUpdate, faceFlux, tables, Direction, Orientation, 1.0, 1.0);
      exahype2::dg::addFaceIntegral<FaceIntegralLayout>(
        nodeUpdate,
        CellNode,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        Direction,
        Orientation,
        1.0,
        1.0
      );

      THEN("The node update equals the update of its node among all nodes and the guard entries keep seven") {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          VALIDATE_EQUALS(nodeUpdate[unknown], everyNodeUpdate[CellNode * NumberOfUnknowns + unknown], unknown);
        }
        for (int entry = NumberOfUnknowns; entry < NumberOfUnknowns + GuardEntries; entry++) {
          VALIDATE_EQUALS(nodeUpdate[entry], GuardValue, entry);
        }
      }
    }
  }
}

UNIT_TEST("The statistics keep the largest eigenvalue and the largest sum of the eigenvalues over the cell extents") {
  GIVEN("Cell extents and two eigenvalue tables that differ per axis and statistics that start from zero") {
    // The largest entry of each table lies on one of the first two axes. The physical maxima are
    // the same in 2D and in 3D.
    constexpr double FirstTable[]  = {3.0, 5.0, 2.0};
    constexpr double SecondTable[] = {7.0, 1.0, 4.0};

    tarch::la::Vector<DIMENSIONS, double> cellSize;
    std::array<double, DIMENSIONS>        firstPhysicalEigenvalues{};
    std::array<double, DIMENSIONS>        secondPhysicalEigenvalues{};
    double                                firstScaledEigenvalue  = 0.0;
    double                                secondScaledEigenvalue = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis)                  = 2.0 * (1 << axis);
      firstPhysicalEigenvalues[axis]  = FirstTable[axis];
      secondPhysicalEigenvalues[axis] = SecondTable[axis];
      firstScaledEigenvalue += firstPhysicalEigenvalues[axis] / cellSize(axis);
      secondScaledEigenvalue += secondPhysicalEigenvalues[axis] / cellSize(axis);
    }

    exahype2::dg::MaxEigenvalueStatistics statistics;

    THEN("Both maxima of the statistics are zero") {
      VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
      VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
    }

    WHEN("The statistics accumulate the first table") {
      statistics.accumulate(cellSize, [&](const int dimension) { return firstPhysicalEigenvalues[dimension]; });

      THEN("The statistics store its largest entry five and its sum of the eigenvalues over the extents") {
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 5.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, firstScaledEigenvalue);
      }

      AND_WHEN("The statistics accumulate the second table") {
        statistics.accumulate(cellSize, [&](const int dimension) { return secondPhysicalEigenvalues[dimension]; });

        THEN("The statistics store its largest entry seven and its larger sum of the eigenvalues over the extents") {
          VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 7.0);
          VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, secondScaledEigenvalue);
        }
      }
    }

    WHEN("The statistics accumulate zero eigenvalues") {
      statistics.accumulate(cellSize, []([[maybe_unused]] const int dimension) { return 0.0; });

      THEN("Both maxima stay zero") {
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

#if PEANO_ASSERTIONS == 0
UNIT_TEST("A negative or an infinite eigenvalue along the first axis becomes both maxima of the statistics") {
  GIVEN("A cell of unit extent and statistics that start from zero") {
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    exahype2::dg::MaxEigenvalueStatistics       statistics;

    WHEN("The statistics accumulate minus two along the first axis and five along the others") {
      statistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? -2.0 : 5.0; });

      THEN("Both maxima are minus two") {
        VALIDATE_EQUALS(statistics.maxPhysicalEigenvalue, -2.0);
        VALIDATE_EQUALS(statistics.maxScaledEigenvalue, -2.0);
      }
    }

    WHEN("The statistics accumulate infinity along the first axis and five along the others") {
      constexpr auto PositiveInfinity = std::numeric_limits<double>::infinity();
      statistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? PositiveInfinity : 5.0; });

      THEN("Both maxima are infinite") {
        VALIDATE_EQUALS(statistics.maxPhysicalEigenvalue, PositiveInfinity);
        VALIDATE_EQUALS(statistics.maxScaledEigenvalue, PositiveInfinity);
      }
    }
  }
}
#endif

UNIT_TEST("The eigenvalue reduction of a cell takes both maxima over its nodes, every axis scaled by its extent") {
  GIVEN("A cell whose extent differs per axis, an unknown that grows over its nodes and an auxiliary variable of 100") {
    constexpr int Data = EigenvalueStubSolver::NumberOfUnknowns + EigenvalueStubSolver::NumberOfAuxiliaryVariables;
    const double  quadratureNodes[2] = {0.25, 0.75};
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    tarch::la::Vector<DIMENSIONS, double>       cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 2.0 * (1 << axis);
    }

    double cellState[EigenvalueCellNodes * Data];
    for (int cellNode = 0; cellNode < EigenvalueCellNodes; cellNode++) {
      const int firstEntry      = cellNode * Data;
      cellState[firstEntry]     = 0.5 * cellNode;
      cellState[firstEntry + 1] = 100.0;
    }

    WHEN("The test reduces the largest eigenvalues of the cell over its nodes") {
      const auto statistics = exahype2::dg::reduceMaxEigenvalues<EigenvalueStubSolver>(
        cellState,
        quadratureNodes,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );

      THEN("Both maxima come from the last node with the largest state and the largest first coordinate") {
        const double lastPosition   = cellCentre(0) - 0.5 * cellSize(0) + quadratureNodes[1] * cellSize(0);
        const double lastState      = 0.5 * (EigenvalueCellNodes - 1);
        double       expectedScaled = 0.0;
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          expectedScaled += (lastState + direction + 0.25 * lastPosition) / cellSize(direction);
        }
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, lastState + (DIMENSIONS - 1) + 0.25 * lastPosition);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, expectedScaled);
      }
    }
  }
}

UNIT_TEST("The finish of a cell postprocesses every node once and reports the wave speeds of the new state if asked") {
  GIVEN("A cell of the postprocessing stub whose unknowns rise by one half per node and whose auxiliaries are 100") {
    constexpr int Data =
      PostprocessingStubSolver::NumberOfUnknowns + PostprocessingStubSolver::NumberOfAuxiliaryVariables;
    const double                                quadratureNodes[2] = {0.25, 0.75};
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(2.0);
    double                                      cellState[EigenvalueCellNodes * Data];
    for (int cellNode = 0; cellNode < EigenvalueCellNodes; cellNode++) {
      cellState[cellNode * Data]     = 0.5 * cellNode;
      cellState[cellNode * Data + 1] = 100.0;
    }

    WHEN("The cell finishes once with and then once without reporting the wave speeds") {
      const auto statistics = exahype2::dg::finishCell<PostprocessingStubSolver, true>(
        cellState,
        quadratureNodes,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );
      const std::vector<double> stateAfterTheFirstFinish(cellState, cellState + EigenvalueCellNodes * Data);
      const auto                unreportedStatistics = exahype2::dg::finishCell<PostprocessingStubSolver, false>(
        cellState,
        quadratureNodes,
        cellCentre,
        cellSize,
        0.0,
        0.0,
        RuntimeParameters{}
      );

      THEN("Each finish adds one to every unknown and keeps every auxiliary variable") {
        for (int cellNode = 0; cellNode < EigenvalueCellNodes; cellNode++) {
          VALIDATE_EQUALS(stateAfterTheFirstFinish[cellNode * Data], 0.5 * cellNode + 1.0, cellNode);
          VALIDATE_EQUALS(stateAfterTheFirstFinish[cellNode * Data + 1], 100.0, cellNode);
          VALIDATE_EQUALS(cellState[cellNode * Data], 0.5 * cellNode + 2.0, cellNode);
          VALIDATE_EQUALS(cellState[cellNode * Data + 1], 100.0, cellNode);
        }
      }

      THEN("The first finish returns the largest postprocessed speeds and the second one the empty statistics") {
        const double                                largestState = 0.5 * (EigenvalueCellNodes - 1) + 1.0;
        const exahype2::dg::MaxEigenvalueStatistics emptyStatistics;
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, largestState);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, DIMENSIONS * largestState / 2.0);
        VALIDATE_EQUALS(unreportedStatistics.maxPhysicalEigenvalue, emptyStatistics.maxPhysicalEigenvalue);
        VALIDATE_EQUALS(unreportedStatistics.maxScaledEigenvalue, emptyStatistics.maxScaledEigenvalue);
      }
    }
  }
}

UNIT_TEST("The mean of a face keeps its bits under the reflection of a tangential axis and in 3D under an axis swap") {
  GIVEN("Face weights of four Gauss nodes per axis, a state of two values and its reflected and swapped copies") {
    constexpr int    NodesPerAxis   = 4;
    constexpr int    NodesPerFace   = exahype2::dg::getNodesPerFace(NodesPerAxis);
    constexpr int    Data           = 2;
    constexpr double AxisWeights[4] = {0.173927422568727, 0.326072577431273, 0.326072577431273, 0.173927422568727};
    constexpr int    LinesOfTheFace = DIMENSIONS == 3 ? NodesPerAxis : 1;

    double weights[NodesPerFace];
    double state[NodesPerFace * Data];
    double reflected[NodesPerFace * Data];
    double swapped[NodesPerFace * Data];
    for (int row = 0; row < LinesOfTheFace; row++) {
      for (int column = 0; column < NodesPerAxis; column++) {
        const int faceNode         = row * NodesPerAxis + column;
        const int reflectedNode    = row * NodesPerAxis + NodesPerAxis - 1 - column;
        const int swappedNode      = DIMENSIONS == 3 ? column * NodesPerAxis + row : faceNode;
        weights[faceNode]          = DIMENSIONS == 3 ? AxisWeights[row] * AxisWeights[column] : AxisWeights[column];
        state[faceNode * Data]     = 0.1 + 0.37 * row + 0.53 * column + 0.01 * row * column;
        state[faceNode * Data + 1] = -0.7 + 0.29 * row - 0.41 * column + 0.03 * row * column;
        reflected[reflectedNode * Data]     = state[faceNode * Data];
        reflected[reflectedNode * Data + 1] = -state[faceNode * Data + 1];
        swapped[swappedNode * Data]         = state[faceNode * Data];
        swapped[swappedNode * Data + 1]     = state[faceNode * Data + 1];
      }
    }

    WHEN("The face mean runs on the state, on its reflected copy and on its swapped copy") {
      constexpr int Lanes = exahype2::dg::getPaddedCountOf(Data);
      double        mean[Lanes];
      double        meanOfReflected[Lanes];
      double        meanOfSwapped[Lanes];
      exahype2::dg::computeMeanOfFace<NodesPerAxis, Data>(mean, state, weights);
      exahype2::dg::computeMeanOfFace<NodesPerAxis, Data>(meanOfReflected, reflected, weights);
      exahype2::dg::computeMeanOfFace<NodesPerAxis, Data>(meanOfSwapped, swapped, weights);

      THEN("The reflected copy returns the first mean and the negated second mean and the swapped copy both means") {
        VALIDATE_EQUALS(meanOfReflected[0], mean[0]);
        VALIDATE_EQUALS(meanOfReflected[1], -mean[1]);
        VALIDATE_EQUALS(meanOfSwapped[0], mean[0]);
        VALIDATE_EQUALS(meanOfSwapped[1], mean[1]);
      }

      THEN("The padding lanes are zero and every mean equals the weighted sum of its value within rounding") {
        for (int lane = Data; lane < Lanes; lane++) {
          VALIDATE_EQUALS(mean[lane], 0.0, lane);
        }
        for (int value = 0; value < Data; value++) {
          double plainSum = 0.0;
          for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
            plainSum += weights[faceNode] * state[faceNode * Data + value];
          }
          VALIDATE_NUMERICAL_EQUALS(mean[value], plainSum, value);
        }
      }
    }
  }
}

UNIT_TEST("The combination takes the larger physical and scaled maxima and keeps a negative entry") {
  GIVEN("The identity of the combination") {
    const auto identity = exahype2::dg::MaxEigenvalueStatistics::identity();

    THEN("Both maxima of the identity are zero") {
      VALIDATE_EQUALS(identity.maxPhysicalEigenvalue, 0.0);
      VALIDATE_EQUALS(identity.maxScaledEigenvalue, 0.0);
    }
  }

  GIVEN("Statistics with the maxima two and eight and statistics with the maxima three and five") {
    exahype2::dg::MaxEigenvalueStatistics left;
    left.maxPhysicalEigenvalue = 2.0;
    left.maxScaledEigenvalue   = 8.0;
    exahype2::dg::MaxEigenvalueStatistics right;
    right.maxPhysicalEigenvalue = 3.0;
    right.maxScaledEigenvalue   = 5.0;

    WHEN("The test combines the two statistics") {
      const auto combined = exahype2::dg::MaxEigenvalueStatistics::combine(left, right);

      THEN("The combination stores the larger physical maximum three and the larger scaled maximum eight") {
        VALIDATE_EQUALS(combined.maxPhysicalEigenvalue, 3.0);
        VALIDATE_EQUALS(combined.maxScaledEigenvalue, 8.0);
      }
    }

    WHEN("The test sets the physical maximum of the second statistics to minus one and combines the two") {
      right.maxPhysicalEigenvalue = -1.0;
      const auto invalid          = exahype2::dg::MaxEigenvalueStatistics::combine(left, right);

      THEN("The combination keeps the negative physical maximum and the larger scaled maximum eight") {
        VALIDATE_EQUALS(invalid.maxPhysicalEigenvalue, -1.0);
        VALIDATE_EQUALS(invalid.maxScaledEigenvalue, 8.0);
      }
    }
  }
}

UNIT_TEST("The state interpolation of a level jump writes the coarse-side half of every fine face alone") {
  GIVEN("The transfer operators and a level jump from a constant coarse side to fine faces of markers") {
    constexpr int    StateElementsPerFace = TransferSetup::FaceNodes * TransferSetup::ValuesPerNode;
    constexpr int    CoarseSide           = 1;
    constexpr double Marker               = -7.0;
    CellPool         faces(2 * StateElementsPerFace, tarch::MemoryLocation::Heap);
    faces.reserve(1 + peano4::SubfacesPerFace);
    peano4::LevelJump levelJump;
    levelJump.coarseFace = 0;
    levelJump.coarseSide = CoarseSide;
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      levelJump.fineFaces[subface] = 1 + subface;
      std::fill_n(faces.getChunk(1 + subface), 2 * StateElementsPerFace, Marker);
    }
    exahype2::dg::unittests::fillTransferConstant<TransferSetup::FaceNodes>(
      faces.getChunk(0) + CoarseSide * StateElementsPerFace
    );

    const exahype2::dg::unittests::TransferOperators operators;

    WHEN("The test interpolates the state of the level jump") {
      exahype2::dg::InterpolateFaceStateHalves<TransferLayout, double>{faces.getView(), operators.prolongation.data()}(
        levelJump
      );

      THEN("The coarse side of every fine face contains the constant and the other side keeps the marker") {
        for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
          const double* const fineFace = faces.getChunk(1 + subface);
          for (int node = 0; node < TransferSetup::FaceNodes; node++) {
            for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
              const int element = node * TransferSetup::ValuesPerNode + value;
              VALIDATE_EQUALS(
                fineFace[CoarseSide * StateElementsPerFace + element],
                exahype2::dg::unittests::getTransferConstantOf(value),
                subface,
                element
              );
              VALIDATE_EQUALS(fineFace[(1 - CoarseSide) * StateElementsPerFace + element], Marker, subface, element);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The state restriction of a level jump writes the fine-side half of the coarse face alone") {
  GIVEN("The transfer operators and a level jump from constant fine sides to a coarse face of markers") {
    constexpr int    StateElementsPerFace = TransferSetup::FaceNodes * TransferSetup::ValuesPerNode;
    constexpr int    CoarseSide           = 0;
    constexpr int    FineSide             = 1 - CoarseSide;
    constexpr double Marker               = -7.0;
    CellPool         faces(2 * StateElementsPerFace, tarch::MemoryLocation::Heap);
    faces.reserve(1 + peano4::SubfacesPerFace);
    std::fill_n(faces.getChunk(0), 2 * StateElementsPerFace, Marker);
    peano4::LevelJump levelJump;
    levelJump.coarseFace = 0;
    levelJump.coarseSide = CoarseSide;
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      levelJump.fineFaces[subface] = 1 + subface;
      std::fill_n(faces.getChunk(1 + subface), 2 * StateElementsPerFace, Marker);
      exahype2::dg::unittests::fillTransferConstant<TransferSetup::FaceNodes>(
        faces.getChunk(1 + subface) + FineSide * StateElementsPerFace
      );
    }

    const exahype2::dg::unittests::TransferOperators operators;

    WHEN("The test restricts the state of the level jump") {
      exahype2::dg::RestrictStateHalves<TransferLayout, double>{faces.getView(), operators.restriction.data()}(
        levelJump
      );

      THEN("The fine side of the coarse face contains the constant and its coarse side keeps the marker") {
        const double* const coarseFace = faces.getChunk(0);
        for (int node = 0; node < TransferSetup::FaceNodes; node++) {
          for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
            const int element = node * TransferSetup::ValuesPerNode + value;
            VALIDATE_NUMERICAL_EQUALS(
              coarseFace[FineSide * StateElementsPerFace + element],
              exahype2::dg::unittests::getTransferConstantOf(value),
              element
            );
            VALIDATE_EQUALS(coarseFace[CoarseSide * StateElementsPerFace + element], Marker, element);
          }
        }
      }
    }
  }
}

UNIT_TEST("The regrid copies every value of a kept cell from its old slot") {
  GIVEN("The transfer operators and a pool with one cell of a linear state") {
    CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
    pool.reserve(1);
    exahype2::dg::unittests::fillLinearTransferProfile<TransferSetup::CellNodes>(pool.getChunk(0), DIMENSIONS, -1);

    const exahype2::dg::unittests::TransferOperators operators;

    WHEN("The transfer copies every node of the cell") {
      double target[ValuesPerTransferCell];
      for (int node = 0; node < Transfer::WorkItemsPerChunk; node++) {
        getTransferOf(pool, operators).copy(0, target, node);
      }

      THEN("The copy contains every value of the slot bitwise") {
        for (int value = 0; value < ValuesPerTransferCell; value++) {
          VALIDATE_EQUALS(target[value], pool.getChunk(0)[value], value);
        }
      }
    }
  }
}

UNIT_TEST("Every new child takes the state of its parent at its own node positions") {
  GIVEN("The transfer operators and a pool with one parent of a linear state") {
    CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
    pool.reserve(1);
    exahype2::dg::unittests::fillLinearTransferProfile<TransferSetup::CellNodes>(pool.getChunk(0), DIMENSIONS, -1);

    const exahype2::dg::unittests::TransferOperators operators;

    WHEN("The transfer prepares the parent and interpolates it onto every node of every child") {
      double prepared[Transfer::PreparedValuesPerChunk];
      for (int item = 0; item < Transfer::PreparationItemsPerChunk; item++) {
        getTransferOf(pool, operators).prepareInterpolation(0, prepared, item);
      }
      double targets[TransferSetup::ChildrenPerCell][ValuesPerTransferCell];
      for (int child = 0; child < TransferSetup::ChildrenPerCell; child++) {
        for (int node = 0; node < Transfer::WorkItemsPerChunk; node++) {
          getTransferOf(pool, operators).interpolateFromParent(0, targets[child], child, node, prepared);
        }
      }

      THEN("Every child contains the linear state at the positions of its nodes") {
        for (int child = 0; child < TransferSetup::ChildrenPerCell; child++) {
          for (int node = 0; node < TransferSetup::CellNodes; node++) {
            double coordinates[DIMENSIONS];
            exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, child);
            for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
              VALIDATE_NUMERICAL_EQUALS(
                targets[child][node * TransferSetup::ValuesPerNode + value],
                exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
                child,
                node,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A coarsened parent takes the state its children represent at its own node positions") {
  GIVEN("The transfer operators and a pool of children with a linear state over their parent") {
    CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
    pool.reserve(TransferSetup::ChildrenPerCell);
    int childSlots[TransferSetup::ChildrenPerCell];
    for (int child = 0; child < TransferSetup::ChildrenPerCell; child++) {
      exahype2::dg::unittests::fillLinearTransferProfile<TransferSetup::CellNodes>(
        pool.getChunk(child),
        DIMENSIONS,
        child
      );
      childSlots[child] = child;
    }

    const exahype2::dg::unittests::TransferOperators operators;

    WHEN("The transfer prepares the children and restricts them onto every node of the parent") {
      double prepared[Transfer::PreparedValuesPerChunk];
      for (int item = 0; item < Transfer::PreparationItemsPerChunk; item++) {
        getTransferOf(pool, operators).prepareRestriction(childSlots, prepared, item);
      }
      double target[ValuesPerTransferCell];
      for (int node = 0; node < Transfer::WorkItemsPerChunk; node++) {
        getTransferOf(pool, operators).restrictFromChildren(childSlots, target, node, prepared);
      }

      THEN("The parent contains the linear state at the positions of its nodes") {
        for (int node = 0; node < TransferSetup::CellNodes; node++) {
          double coordinates[DIMENSIONS];
          exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, -1);
          for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              target[node * TransferSetup::ValuesPerNode + value],
              exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
              node,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The state extrapolation of a chain of cells writes the same faces on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells, cell states that differ per entry and zero face states") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      cellStates =
      exahype2::dg::unittests::getRamp(Setup::AllCellStatesCardinality, 0.4, 0.013);
    const std::vector<double> faceStates(geometry.getNumberOfFaceStateEntries(), 0.0);

    WHEN("Every cell extrapolates its state to its faces on the device and on the host") {
      const std::vector<double> deviceResult = getExtrapolatedStatesOf(operators, geometry, cellStates, faceStates);

      std::vector<double> hostResult = faceStates;
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        double* facesOfCell[peano4::FacesPerCell];
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          facesOfCell[face] = hostResult.data()
            + geometry.faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceStateCardinality
            + (face < DIMENSIONS ? Setup::FaceSideStateCardinality : 0);
        }
        const int cellStateOffset = cell * Setup::CellStateCardinality;
        exahype2::dg::extrapolateStateToFaces<Stub>(
          facesOfCell,
          cellStates.data() + cellStateOffset,
          operators.leftFaceCoefficients.data(),
          operators.rightFaceCoefficients.data()
        );
      }

      THEN("The face states of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The flux extrapolation of a chain of cells writes the same faces on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells, cell states that differ per entry and zero face fluxes") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      cellStates =
      exahype2::dg::unittests::getRamp(Setup::AllCellStatesCardinality, 0.3, 0.017);
    const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

    WHEN("Every cell extrapolates its flux to its faces on the device and on the host") {
      const std::vector<double> deviceResult = getExtrapolatedFluxesOf(operators, geometry, cellStates, faceFluxes);

      std::vector<double> hostResult = faceFluxes;
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        double* facesOfCell[peano4::FacesPerCell];
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          facesOfCell[face] = hostResult.data()
            + geometry.faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceFluxCardinality
            + (face < DIMENSIONS ? Setup::FaceSideFluxCardinality : 0);
        }
        const int cellStateOffset  = cell * Setup::CellStateCardinality;
        const int coordinateOffset = cell * DIMENSIONS;
        double    nodalFluxes[DIMENSIONS * Setup::StageCardinality];
        exahype2::dg::evaluateNodalFluxes<Stub>(
          nodalFluxes,
          cellStates.data() + cellStateOffset,
          operators.quadratureNodes.data(),
          operators.derivativeOperator.data(),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
          Setup::TimeStamp,
          Setup::TimeStepSize,
          RuntimeParameters{}
        );
        exahype2::dg::extrapolateFluxToFaces<Stub>(
          facesOfCell,
          nodalFluxes,
          operators.leftFaceCoefficients.data(),
          operators.rightFaceCoefficients.data()
        );
      }

      THEN("The face fluxes of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The flux evaluation on the faces of a chain of cells gives the same fluxes on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells, face states that differ per entry and zero face fluxes") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      faceStates =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.7, 0.009);
    const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

    WHEN("Both sides of every work face evaluate their flux on the device and on the host") {
      const std::vector<double> deviceResult = getFluxesOnFacesOf(operators, geometry, faceStates, faceFluxes);

      std::vector<double> hostResult = faceFluxes;
      for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
        const int   faceNumber       = geometry.workFaceNumbers[face];
        const int   cell             = geometry.workCellIndices[face];
        const int   coordinateOffset = cell * DIMENSIONS;
        const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
        auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
        for (int side = 0; side < 2; side++) {
          const int sideFluxOffset = side * Setup::FaceSideFluxCardinality;
          exahype2::dg::evaluateFluxOnFace<Stub>(
            faceFlux + sideFluxOffset,
            faceState,
            faceState + Setup::FaceSideStateCardinality,
            side,
            operators.quadratureNodes.data(),
            exahype2::dg::unittests::getFaceCentreOf(
              cell,
              faceNumber,
              geometry.cellCentres.data(),
              geometry.cellSizes.data()
            ),
            tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
            Setup::TimeStamp,
            Setup::TimeStepSize,
            faceNumber % DIMENSIONS,
            RuntimeParameters{}
          );
        }
      }

      THEN("The face fluxes of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The boundary conditions of a chain of cells give the same face states on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells and face states that differ per entry") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      faceStates =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.2, 0.011);

    WHEN("Every work face applies the boundary conditions of the solver on the device and on the host") {
      const std::vector<double> deviceResult = getBoundaryConditionsOf(operators, geometry, faceStates);

      std::vector<double> hostResult = faceStates;
      for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
        const int faceNumber       = geometry.workFaceNumbers[face];
        const int cell             = geometry.workCellIndices[face];
        const int coordinateOffset = cell * DIMENSIONS;
        exahype2::dg::applyBoundaryConditions<Stub>(
          hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality,
          operators.quadratureNodes.data(),
          exahype2::dg::unittests::getFaceCentreOf(
            cell,
            faceNumber,
            geometry.cellCentres.data(),
            geometry.cellSizes.data()
          ),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
          Setup::TimeStamp,
          faceNumber,
          RuntimeParameters{}
        );
      }

      THEN("The face states of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The Riemann solve on the faces of a chain of cells gives the same fluxes on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells and face states and fluxes that differ per entry") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      faceStates =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.6, 0.007);
    const std::vector<double> faceFluxes =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceFluxEntries(), -0.3, 0.019);

    WHEN("Every work face solves its Riemann problem from the fluxes it overwrites on the device and on the host") {
      const std::vector<double> deviceResult = getRiemannFluxesOf(operators, geometry, faceStates, faceFluxes);

      std::vector<double> hostResult = faceFluxes;
      for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
        const int   faceNumber       = geometry.workFaceNumbers[face];
        const int   cell             = geometry.workCellIndices[face];
        const int   coordinateOffset = cell * DIMENSIONS;
        const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
        auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
        exahype2::dg::solveRiemannProblem<Stub>(
          faceFlux,
          faceFlux + Setup::FaceSideFluxCardinality,
          faceState,
          faceState + Setup::FaceSideStateCardinality,
          operators.faceQuadratureWeights.data(),
          exahype2::dg::unittests::getFaceCentreOf(
            cell,
            faceNumber,
            geometry.cellCentres.data(),
            geometry.cellSizes.data()
          ),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
          Setup::TimeStamp,
          Setup::TimeStepSize,
          faceNumber % DIMENSIONS,
          RuntimeParameters{}
        );
      }

      THEN("The face fluxes of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The linear Riemann solve on a chain of cells gives the same fluxes on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells, face states that differ per entry and zero face fluxes") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      faceStates =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.9, 0.005);
    const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

    WHEN("Every work face solves its linear Riemann problem on the device and on the host") {
      const std::vector<double> deviceResult = getLinearRiemannFluxesOf(operators, geometry, faceStates, faceFluxes);

      std::vector<double> hostResult = faceFluxes;
      for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
        const int   faceNumber       = geometry.workFaceNumbers[face];
        const int   cell             = geometry.workCellIndices[face];
        const int   coordinateOffset = cell * DIMENSIONS;
        const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
        auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
        exahype2::dg::solveLinearRiemannProblem<Stub>(
          faceFlux,
          faceFlux + Setup::FaceSideFluxCardinality,
          faceState,
          faceState + Setup::FaceSideStateCardinality,
          operators.faceQuadratureWeights.data(),
          exahype2::dg::unittests::getFaceCentreOf(
            cell,
            faceNumber,
            geometry.cellCentres.data(),
            geometry.cellSizes.data()
          ),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
          Setup::TimeStamp,
          Setup::TimeStepSize,
          faceNumber % DIMENSIONS,
          RuntimeParameters{}
        );
      }

      THEN("The face fluxes of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The face integrals of a chain of cells give the same estimates on the device as on the host") {
  GIVEN("The kernel operators, a chain of cells and right-hand side estimates and face fluxes that differ per entry") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      rightHandSideEstimates =
      exahype2::dg::unittests::getRamp(Setup::AllRightHandSidesCardinality, 0.1, 0.003);
    const std::vector<double> faceFluxes =
      exahype2::dg::unittests::getRamp(geometry.getNumberOfFaceFluxEntries(), -0.2, 0.009);

    WHEN("Every cell adds the integrals of its faces to its estimate on the device and on the host") {
      const std::vector<double> deviceResult =
        getFaceIntegralsOf(operators, geometry, rightHandSideEstimates, faceFluxes);

      std::vector<double> hostResult = rightHandSideEstimates;
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        const int   cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
        const int   coordinateOffset = cell * DIMENSIONS;
        auto* const cellUpdate       = hostResult.data() + cellUpdateOffset;
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          const double inverseCellSizeAlongNormal = 1.0 / geometry.cellSizes[coordinateOffset + direction];

          const auto* const lowerFaceFlux = faceFluxes.data()
            + geometry.faceIndices[cell * peano4::FacesPerCell + direction] * Setup::FaceFluxCardinality
            + Setup::FaceSideFluxCardinality;
          const auto* const upperFaceFlux = faceFluxes.data()
            + geometry.faceIndices[cell * peano4::FacesPerCell + direction + DIMENSIONS] * Setup::FaceFluxCardinality;
          for (int cellNode = 0; cellNode < Setup::NodesPerCell; cellNode++) {
            auto* const nodeUpdate = cellUpdate + static_cast<std::ptrdiff_t>(cellNode) * Setup::NumberOfUnknowns;
            exahype2::dg::addFaceIntegral<Stub>(
              nodeUpdate,
              cellNode,
              lowerFaceFlux,
              operators.faceQuadratureWeights.data(),
              operators.inverseCellQuadratureWeights.data(),
              operators.leftFaceCoefficients.data(),
              operators.rightFaceCoefficients.data(),
              direction,
              0,
              inverseCellSizeAlongNormal,
              Setup::TimeStepSize
            );
            exahype2::dg::addFaceIntegral<Stub>(
              nodeUpdate,
              cellNode,
              upperFaceFlux,
              operators.faceQuadratureWeights.data(),
              operators.inverseCellQuadratureWeights.data(),
              operators.leftFaceCoefficients.data(),
              operators.rightFaceCoefficients.data(),
              direction,
              1,
              inverseCellSizeAlongNormal,
              Setup::TimeStepSize
            );
          }
        }
      }

      THEN("The estimates of the device and of the host agree within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceResult, hostResult);
      }
    }
  }
}

UNIT_TEST("The parallel reduction of the largest eigenvalues on the device equals the serial reduction on the host") {
  GIVEN("The kernel operators, a chain of cells and cell states that differ per entry") {
    const exahype2::dg::unittests::KernelOperators operators;
    const exahype2::dg::unittests::CellChain       geometry;
    const std::vector<double>                      cellStates =
      exahype2::dg::unittests::getRamp(Setup::AllCellStatesCardinality, 0.25, 0.031);

    WHEN("The test reduces the largest eigenvalues per cell on the device and combines them per cell on the host") {
      const exahype2::dg::MaxEigenvalueStatistics deviceResult = getMaxEigenvaluesOf(operators, geometry, cellStates);

      exahype2::dg::MaxEigenvalueStatistics hostResult = exahype2::dg::MaxEigenvalueStatistics::identity();
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        const int cellStateOffset  = cell * Setup::CellStateCardinality;
        const int coordinateOffset = cell * DIMENSIONS;
        hostResult                 = exahype2::dg::MaxEigenvalueStatistics::combine(
          hostResult,
          exahype2::dg::reduceMaxEigenvalues<Stub>(
            cellStates.data() + cellStateOffset,
            operators.quadratureNodes.data(),
            tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
            tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
            Setup::TimeStamp,
            Setup::TimeStepSize,
            RuntimeParameters{}
          )
        );
      }

      THEN("Both maxima of the device equal those of the host and the physical maximum is positive") {
        VALIDATE_NUMERICAL_EQUALS(deviceResult.maxScaledEigenvalue, hostResult.maxScaledEigenvalue);
        VALIDATE_NUMERICAL_EQUALS(deviceResult.maxPhysicalEigenvalue, hostResult.maxPhysicalEigenvalue);
        VALIDATE(hostResult.maxPhysicalEigenvalue > 0.0);
      }
    }
  }
}
