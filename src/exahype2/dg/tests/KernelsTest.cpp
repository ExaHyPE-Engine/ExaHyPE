// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsTest.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "exahype2/dg/tests/TransferTestData.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

namespace {
  TARGET_DECLARE
  /**
   * Stands in for a generated solver of order two with two unknowns and one auxiliary variable.
   */
  struct Order2Layout {
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
   * A flux that adds a multiple of the state, so a flux buffer that is not cleared shows.
   */
  struct FluxSolver final : Order2Layout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
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

  /**
   * A diffusive flux that sums one plus the axis number times the gradient along that axis.
   */
  struct DiffusiveFluxSolver final : Order2Layout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
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
      const int                                                     direction
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

  // Gauss-Legendre nodes of three points on the unit interval.
  const double QuadratureNodes[NodesPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};

  /**
   * Returns the value of the Lagrange basis function of node at x.
   */
  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  /**
   * Returns the derivative of the Lagrange basis function of node at x.
   */
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
   * Returns the cell node that carries the face node of the face along direction in layer normalIndex.
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

  /**
   * Returns the position of a face node: The face centre along the normal and, along the
   * tangential axes in ascending order, the quadrature node of the lexicographic face index.
   */
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

  void fillInnerHalf(double (&faceState)[2 * FaceNodes * NumberOfData], const int innerSide) {
    for (int entry = 0; entry < 2 * FaceNodes * NumberOfData; entry++) {
      faceState[entry] = -1.0;
    }
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      faceState[innerSide * FaceNodes * NumberOfData + entry] = 0.25 * entry;
    }
  }

  TARGET_DECLARE
  /**
   * Stands in for a generated solver of order one with two unknowns and Auxiliary auxiliary variables.
   */
  template <int Auxiliary>
  struct Order1Layout {
    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 2;
    static constexpr int NumberOfAuxiliaryVariables = Auxiliary;
  };

  constexpr int RiemannUnknowns  = Order1Layout<0>::NumberOfUnknowns;
  constexpr int RiemannFaceNodes = exahype2::dg::getNodesPerFace(Order1Layout<0>::DGOrder + 1);
  TARGET_DECLARE_END

  TARGET_DECLARE
  /**
   * Reports the first entry of the state as the wave speed, so a test steers the wave speed of each side.
   */
  struct FirstEntryEigenvalue {
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    ) {
      return state[0];
    }
  };

  /**
   * A flux that adds the state, so a flux buffer the kernel does not clear shows.
   */
  struct AccumulatingFlux {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        fluxValues[unknown] += state[unknown];
      }
    }
  };

  /**
   * A nonconservative product that adds the state jump of the unknowns.
   */
  struct JumpProduct {
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        productValues[unknown] += stateJump[unknown];
      }
    }
  };

  /**
   * A nonconservative product that reads the jump of the first auxiliary variable, as a bathymetry does.
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
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < RiemannUnknowns; unknown++) {
        productValues[unknown] += (1.0 + unknown) * stateJump[RiemannUnknowns];
      }
    }
  };

  /**
   * Doubles whatever it is handed, which shows that the matrix ran.
   */
  struct DoublingMaterial {
    static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         values
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
   * Weights that sum to one, so a uniform state averages to itself.
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
   * Stands in for a generated solver of order one with one unknown; Linear picks the Riemann solver.
   */
  template <bool Linear>
  struct FaceLayout {
    static constexpr int  DGOrder                    = 1;
    static constexpr int  NumberOfUnknowns           = FaceUnknowns;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = Linear;
  };

  /**
   * Returns one number whose decimal digits hold the arguments a face kernel passes to the solver.
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
   * Returns the wave speed at the time stamp of the tests only and a constant outside state.
   */
  struct FaceTerms {
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction
    ) {
      return timeStamp == TimeStamp ? WaveSpeed : 0.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      [[maybe_unused]] const double* NOALIAS const                  insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    direction
    ) {
      for (int value = 0; value < FaceData; value++) {
        outsideState[value] = OutsideValue;
      }
    }
  };

  struct FaceSolver final : FaceLayout<false>, FaceTerms {};
  struct LinearFaceSolver final : FaceLayout<true>, FaceTerms {};

  /**
   * Its own Riemann solver adds the encoded face arguments to the fluxes of both sides.
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
      double* NOALIAS const                                         rightFlux
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

  /**
   * Holds the two halves of one face: The face states and the face fluxes of both sides.
   */
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

  /**
   * Runs the Riemann solve of one face with the Riemann solver of Solver.
   */
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
      boundaryFaceNumber
    );
  }

  /**
   * Stands in for a generated solver of order two with two unknowns; a face integral reads no auxiliary variable.
   */
  struct FaceIntegralLayout {
    static constexpr int DGOrder          = 2;
    static constexpr int NumberOfUnknowns = 2;
  };

  struct Tables final {
    double faceQuadratureWeights[FaceNodes]{};
    double inverseCellQuadratureWeights[CellNodes]{};
    double leftFaceCoefficients[NodesPerAxis]{};
    double rightFaceCoefficients[NodesPerAxis]{};
  };

  // Distinct, non-symmetric entries, so an index mix-up cannot cancel out.
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

  constexpr int EigenvalueCellNodes = exahype2::dg::getNodesPerCell(2);

  TARGET_DECLARE
  /**
   * Reports the first unknown plus the axis number plus a quarter of the first coordinate, so
   * the state, the direction and the node position all reach the result.
   */
  struct EigenvalueStubSolver final {
    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 1;

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* NOALIAS const                                   state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      const int                                                     direction
    ) {
      return state[0] + direction + 0.25 * position(0);
    }
  };
  TARGET_DECLARE_END

  using TransferSetup = exahype2::dg::tests::TransferSetup;

  constexpr int ValuesPerTransferCell = TransferSetup::CellNodes * TransferSetup::ValuesPerNode;

  /**
   * Stands in for a generated solver with the node layout of the transfer setup and every value an unknown.
   */
  struct TransferLayout final {
    static constexpr int DGOrder                    = TransferSetup::NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = TransferSetup::ValuesPerNode;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  using CellPool    = tarch::accelerator::ChunkedMemoryPool<double>;
  using RegridCells = exahype2::dg::RegridCells<TransferLayout, double>;

  RegridCells getRegridOf(const CellPool& pool, const exahype2::dg::tests::TransferOperators& operators) {
    return RegridCells{pool.getView(), operators.prolongation.data(), operators.restriction.data()};
  }

  using Setup = exahype2::dg::tests::KernelTestSetup;
  using Stub  = exahype2::dg::tests::StubSolver;

  TARGET_DECLARE
  /**
   * Writes the state of one cell into the half of each of its faces that lies on its own side.
   */
  struct ExtrapolateStateOfCell final {
    const double*        cellStates{nullptr};
    double*              faceStates{nullptr};
    const std::uint64_t* faceIndices{nullptr};
    const double*        leftFaceCoefficients{nullptr};
    const double*        rightFaceCoefficients{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      // The two cells at a face never touch the same entries.
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
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  cellStates,
    const std::vector<double>&                  faceStates
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceRightFaceCoefficients(operators.rightFaceCoefficients);

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
   * Writes the flux of one cell into the half of each of its faces that lies on its own side.
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
      exahype2::dg::extrapolateFluxToFaces<Stub>(
        facesOfCell,
        cellStates + cellStateOffset,
        leftFaceCoefficients,
        rightFaceCoefficients,
        quadratureNodes,
        derivativeOperator,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getExtrapolatedFluxesOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  cellStates,
    const std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceRightFaceCoefficients(operators.rightFaceCoefficients);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceDerivativeOperator(operators.derivativeOperator);

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
   * Writes the flux of one side of one work face; the two sides write disjoint halves of the buffer.
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
        exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getFluxesOnFacesOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  faceStates,
    const std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

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
        exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        faceNumber
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getBoundaryConditionsOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  faceStates
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

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
  /**
   * Writes the Riemann flux of both sides of one work face.
   */
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
        exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getRiemannFluxesOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  faceStates,
    const std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

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
  /**
   * Writes the flux of the linear Riemann problem of both sides of one work face.
   */
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
        exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, cellCentres, cellSizes),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  };
  TARGET_DECLARE_END

  std::vector<double> getLinearRiemannFluxesOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  faceStates,
    const std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
    const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

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
  /**
   * Adds the integrals of the two faces along every axis of one cell to its estimate.
   */
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
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  rightHandSideEstimates,
    const std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBuffer<double>        deviceRightHandSideEstimates(rightHandSideEstimates);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);
    const exahype2::dg::tests::DeviceBuffer<double>        deviceInverseCellQuadratureWeights(
      operators.inverseCellQuadratureWeights
    );
    const exahype2::dg::tests::DeviceBuffer<double> deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
    const exahype2::dg::tests::DeviceBuffer<double> deviceRightFaceCoefficients(operators.rightFaceCoefficients);

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
  /**
   * Raises the maximum to the eigenvalue statistics of one cell.
   */
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
          Setup::TimeStepSize
        )
      );
    }
  };
  TARGET_DECLARE_END

  exahype2::dg::MaxEigenvalueStatistics getMaxEigenvaluesOf(
    const exahype2::dg::tests::KernelOperators& operators,
    const exahype2::dg::tests::CellChain&       geometry,
    const std::vector<double>&                  cellStates
  ) {
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceQuadratureNodes(operators.quadratureNodes);

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

exahype2::dg::tests::KernelsTest::KernelsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::KernelsTest") {}

void
exahype2::dg::tests::KernelsTest::run() {
  testMethod(testFaceBuffersAreClearedBeforeTheProjection);
  testMethod(testAConstantStateReachesEveryFace);
  testMethod(testEachFaceNodeSeesOnlyItsOwnLine);
  testMethod(testTheTwoEndsUseTheirOwnCoefficients);
  testMethod(testAStateOfFloatReachesFacesOfDouble);
  testMethod(testFluxIsEvaluatedAtCellNodesBeforeExtrapolation);
  testMethod(testASystemWithoutFluxLeavesZeroesBehind);
  testMethod(testDiffusiveFluxIsExtrapolatedWithTheNodalGradients);
  testMethod(testTheOuterHalfIsFilledFromTheInnerHalf);
  testMethod(testEveryNodeSeesItsOwnPositionOnTheFace);
  testMethod(testFluxOnAFaceComesFromTheFaceState);
  testMethod(testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal);
  testMethod(testEqualStatesLeaveHalfTheFluxJumpOnBothSides);
  testMethod(testEqualSidesLeaveZerosBitwise);
  testMethod(testJumpTermScalesWithTheLargerWaveSpeed);
  testMethod(testTheTwoDifferencesAddUpToTheFluxJump);
  testMethod(testNonconservativeProductEntersBothSidesAlike);
  testMethod(testNonconservativeProductReadsTheAuxiliaryJump);
  testMethod(testLinearFormBuildsTheFluxFromTheStateJump);
  testMethod(testLinearFormAddsTheNonconservativeProduct);
  testMethod(testLinearFormAppliesTheMaterialParameterMatrix);
  testMethod(testAnInteriorFaceHoldsTheDifferencesToItsOwnFluxes);
  testMethod(testABoundaryFaceTakesTheBoundaryState);
  testMethod(testTheSolversRiemannSolverReceivesTheFace);
  testMethod(testTheLinearRiemannSolverWritesOppositeDissipations);
  testMethod(testZeroFluxLeavesUpdateUntouched);
  testMethod(testEachCellNodeReceivesItsOwnFaceNode);
  testMethod(testUpdateStridesByTheUnknowns);
  testMethod(testBothOrientationsEnterWithTheSameSign);
  testMethod(testWeightsAndCoefficientsEnterAsAProduct);
  testMethod(testANodeReceivesOnlyItsOwnUpdate);
  testMethod(testAnisotropicAccumulation);
#if PEANO_ASSERTIONS == 0
  testMethod(testInvalidEigenvalueAccumulation);
#endif
  testMethod(testReduceMaxEigenvaluesOverTheCellNodes);
  testMethod(testTheReductionKeepsTheLargerAndAnInvalidObservation);
  testMethod(testTheStateInterpolationFillsTheCoarseSideOfTheFineFaces);
  testMethod(testTheStateRestrictionFillsTheFineSideOfTheCoarseFace);
  testMethod(testCopyKeepsEveryValue);
  testMethod(testInterpolationFromTheParentReproducesALinearState);
  testMethod(testRestrictionFromTheChildrenReproducesALinearState);
  testMethod(testExtrapolateStateToFacesOnTheDevice);
  testMethod(testExtrapolateFluxToFacesOnTheDevice);
  testMethod(testEvaluateFluxOnFaceOnTheDevice);
  testMethod(testApplyBoundaryConditionsOnTheDevice);
  testMethod(testSolveRiemannProblemOnTheDevice);
  testMethod(testSolveLinearRiemannProblemOnTheDevice);
  testMethod(testAddFaceIntegralOnTheDevice);
  testMethod(testReduceMaxEigenvaluesOnTheDevice);
}

void
exahype2::dg::tests::KernelsTest::testFaceBuffersAreClearedBeforeTheProjection() {
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

  exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients, coefficients);
  exahype2::dg::extrapolateFluxToFaces<FluxSolver>(
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
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      validateNumericalEquals(buffers.state[face][entry], 0.0);
    }
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      validateNumericalEquals(buffers.flux[face][entry], 0.0);
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testAConstantStateReachesEveryFace() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  // Constants without an exact binary representation, so a plain product leaves rounding behind.
  double cellState[CellNodes * NumberOfData] = {};
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.1 * value;
    }
  }

  const EndCoefficients coefficients;
  exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        validateEqualsWithParams3(
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

void
exahype2::dg::tests::KernelsTest::testEachFaceNodeSeesOnlyItsOwnLine() {
  double leftCoefficients[NodesPerAxis];
  double rightCoefficients[NodesPerAxis];
  for (int index = 0; index < NodesPerAxis; index++) {
    leftCoefficients[index]  = 1.0 + index;
    rightCoefficients[index] = 0.0;
  }

  for (int markedCellNode = 0; markedCellNode < CellNodes; markedCellNode++) {
    FaceBuffers buffers;
    double*     state[peano4::FacesPerCell];
    double*     flux[peano4::FacesPerCell];
    collectPointers(buffers, state, flux);

    double    cellState[CellNodes * NumberOfData] = {};
    const int markedEntry                         = markedCellNode * NumberOfData;
    cellState[markedEntry]                        = 1.0;

    exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, leftCoefficients, rightCoefficients);

    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          if (getCellNodeOf(direction, faceNode, normalIndex) == markedCellNode) {
            expected = leftCoefficients[normalIndex];
          }
        }
        const int faceEntry = faceNode * NumberOfData;
        validateNumericalEquals(state[direction][faceEntry], expected);
      }
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testTheTwoEndsUseTheirOwnCoefficients() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  // A state that is linear along every axis with the slope two plus the axis number.
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
  exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

  // The lower face holds the line's value at zero, the upper face the value at one.
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      const int firstCellNode = getCellNodeOf(direction, faceNode, 0);
      for (int value = 0; value < NumberOfData; value++) {
        const double expectedLower =
          cellState[firstCellNode * NumberOfData + value] - (2.0 + direction) * QuadratureNodes[0];
        validateNumericalEqualsWithParams3(
          state[direction][faceNode * NumberOfData + value],
          expectedLower,
          direction,
          faceNode,
          value
        );
        validateNumericalEqualsWithParams3(
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

/**
 * A constant state of float reaches every face node of double with its float value unchanged.
 */
void
exahype2::dg::tests::KernelsTest::testAStateOfFloatReachesFacesOfDouble() {
  float cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = static_cast<float>(1.0 / 3.0 + 0.1 * value);
    }
  }
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  const EndCoefficients coefficients;
  exahype2::dg::extrapolateStateToFaces<Order2Layout>(state, cellState, coefficients.left, coefficients.right);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        validateEqualsWithParams3(
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

void
exahype2::dg::tests::KernelsTest::testFluxIsEvaluatedAtCellNodesBeforeExtrapolation() {
  const EndCoefficients endCoefficients;
  const double* const   coefficients = endCoefficients.left;

  double cellState[CellNodes * NumberOfData] = {};
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      cellState[cellNode * NumberOfData + unknown] = 1.0 + cellNode + 0.25 * unknown;
    }
  }

  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);
  const double                                quadratureNodes[NodesPerAxis] = {};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::extrapolateFluxToFaces<FluxSolver>(
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
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int cellNode = getCellNodeOf(direction, faceNode, normalIndex);
          expected += (direction + 1.0) * cellState[cellNode * NumberOfData + unknown] * coefficients[normalIndex];
        }
        validateNumericalEquals(flux[direction][faceNode * NumberOfUnknowns + unknown], expected);
        validateNumericalEquals(flux[direction + DIMENSIONS][faceNode * NumberOfUnknowns + unknown], expected);
      }
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testASystemWithoutFluxLeavesZeroesBehind() {
  FaceBuffers buffers;
  double*     state[peano4::FacesPerCell];
  double*     flux[peano4::FacesPerCell];
  collectPointers(buffers, state, flux);

  double cellState[CellNodes * NumberOfData] = {};
  double coefficients[NodesPerAxis];
  double quadratureNodes[NodesPerAxis] = {};
  for (int index = 0; index < NodesPerAxis; index++) {
    coefficients[index] = 1.0 + index;
  }
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::extrapolateFluxToFaces<Order2Layout>(
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
    for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
      validateNumericalEquals(flux[face][entry], 0.0);
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testDiffusiveFluxIsExtrapolatedWithTheNodalGradients() {
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

  // The reference gradient applies the operator line by line, so the kernel's
  // block layout is checked against a direct evaluation.
  double gradients[DIMENSIONS][CellNodes * NumberOfData] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int line = 0; line < FaceNodes; line++) {
      for (int targetIndex = 0; targetIndex < NodesPerAxis; targetIndex++) {
        const int targetNode = getCellNodeOf(axis, line, targetIndex);
        for (int sourceIndex = 0; sourceIndex < NodesPerAxis; sourceIndex++) {
          const int    sourceNode = getCellNodeOf(axis, line, sourceIndex);
          const double weight = derivativeOperator.entries[targetIndex * NodesPerAxis + sourceIndex] / cellSize(axis);
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
  exahype2::dg::extrapolateFluxToFaces<DiffusiveFluxSolver>(
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

void
exahype2::dg::tests::KernelsTest::testTheOuterHalfIsFilledFromTheInnerHalf() {
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    // The cell behind a lower face wrote the upper half, and the other way round.
    const int innerSide = faceNumber < DIMENSIONS ? 1 : 0;
    const int outerSide = 1 - innerSide;
    double    faceState[2 * FaceNodes * NumberOfData];
    fillInnerHalf(faceState, innerSide);

    exahype2::dg::applyBoundaryConditions<RecordingSolver>(
      faceState,
      BoundaryQuadratureNodes,
      getFaceCentre(faceNumber),
      getCellSize(),
      BoundaryTimeStamp,
      faceNumber
    );

    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      const auto position = getNodePositionOf(faceNumber, faceNode);
      for (int value = 0; value < NumberOfData; value++) {
        const int    innerEntry = (innerSide * FaceNodes + faceNode) * NumberOfData + value;
        const int    outerEntry = (outerSide * FaceNodes + faceNode) * NumberOfData + value;
        const double expected   = 2.0 * faceState[innerEntry] + position(faceNumber % DIMENSIONS)
          + 10.0 * position((faceNumber % DIMENSIONS + 1) % DIMENSIONS) + BoundaryTimeStamp;
        validateNumericalEqualsWithParams2(
          faceState[innerEntry],
          0.25 * (faceNode * NumberOfData + value),
          faceNumber,
          faceNode
        );
        validateNumericalEqualsWithParams2(faceState[outerEntry], expected, faceNumber, faceNode);
      }
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testEveryNodeSeesItsOwnPositionOnTheFace() {
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int innerSide                               = faceNumber < DIMENSIONS ? 1 : 0;
    const int outerSide                               = 1 - innerSide;
    double    faceState[2 * FaceNodes * NumberOfData] = {};

    exahype2::dg::applyBoundaryConditions<RecordingSolver>(
      faceState,
      BoundaryQuadratureNodes,
      getFaceCentre(faceNumber),
      getCellSize(),
      0.0,
      faceNumber
    );

    // A zero inner state leaves the position alone in the outer state, and the
    // normal coordinate of every node is the face centre's.
    const int direction = faceNumber % DIMENSIONS;
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      const auto   position   = getNodePositionOf(faceNumber, faceNode);
      const int    outerEntry = (outerSide * FaceNodes + faceNode) * NumberOfData;
      const double recorded   = faceState[outerEntry];
      validateNumericalEqualsWithParams2(
        recorded,
        position(direction) + 10.0 * position((direction + 1) % DIMENSIONS),
        faceNumber,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        position(direction),
        getFaceCentre(faceNumber)(direction),
        faceNumber,
        faceNode
      );
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testFluxOnAFaceComesFromTheFaceState() {
  const double                                quadratureNodes[NodesPerAxis] = {0.1, 0.5, 0.9};
  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  double leftState[FaceNodes * NumberOfData];
  double rightState[FaceNodes * NumberOfData];
  for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
    leftState[entry]  = 1.0 + 0.5 * entry;
    rightState[entry] = -2.0 + 0.25 * entry;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int side = 0; side < 2; side++) {
      const double* const faceState = side == 0 ? leftState : rightState;
      double              faceFlux[FaceNodes * NumberOfUnknowns];
      for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
        faceFlux[entry] = 9.0;
      }

      exahype2::dg::evaluateFluxOnFace<FluxSolver>(
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
      for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
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

      exahype2::dg::evaluateFluxOnFace<Order2Layout>(
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
      for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
        validateNumericalEquals(faceFlux[entry], 0.0);
      }
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal() {
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

  // Both sides see the same jump, so both fluxes hold the stub's multiple of
  // the jump over the cell extent along the normal axis.
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int side = 0; side < 2; side++) {
      double faceFlux[FaceNodes * NumberOfUnknowns] = {};
      exahype2::dg::evaluateFluxOnFace<DiffusiveFluxSolver>(
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
      for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
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

/**
 * Two equal states average the two fluxes, so each side keeps half the flux jump.
 */
void
exahype2::dg::tests::KernelsTest::testEqualStatesLeaveHalfTheFluxJumpOnBothSides() {
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
    0
  );

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      validateNumericalEquals(leftFlux[faceNode * Unknowns + unknown], 1.0);
      validateNumericalEquals(rightFlux[faceNode * Unknowns + unknown], 1.0);
    }
  }
}

/**
 * Two sides with the same states and fluxes leave zeros bitwise, with and without a nonconservative product.
 */
void
exahype2::dg::tests::KernelsTest::testEqualSidesLeaveZerosBitwise() {
  constexpr int Unknowns = ProductSolverWithAuxiliary::NumberOfUnknowns;
  constexpr int Data     = Unknowns + ProductSolverWithAuxiliary::NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[RiemannFaceNodes];
  fillNormalisedWeights(faceQuadratureWeights);

  // Values without an exact binary representation, so a plain product leaves rounding behind.
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
    0
  );

  for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
    validateEqualsWithParams1(leftFlux[entry], 0.0, entry);
    validateEqualsWithParams1(rightFlux[entry], 0.0, entry);
  }
}

void
exahype2::dg::tests::KernelsTest::testJumpTermScalesWithTheLargerWaveSpeed() {
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
    0
  );

  // The right side averages to 5, the left one to 3, so the larger speed wins; the own fluxes
  // are zero, so the left side holds the numerical flux and the right side its negative.
  constexpr double ExpectedMaxEigenvalue = 5.0;
  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    const int firstEntry = faceNode * Unknowns;
    validateNumericalEquals(leftFlux[firstEntry], 0.5 * ExpectedMaxEigenvalue * (3.0 - 5.0));
    validateNumericalEquals(leftFlux[firstEntry + 1], 0.5 * ExpectedMaxEigenvalue * (1.0 - 4.0));
    validateNumericalEquals(rightFlux[firstEntry], -0.5 * ExpectedMaxEigenvalue * (3.0 - 5.0));
    validateNumericalEquals(rightFlux[firstEntry + 1], -0.5 * ExpectedMaxEigenvalue * (1.0 - 4.0));
  }
}

/**
 * Without a nonconservative product the two stored differences add up to the jump of the own fluxes.
 */
void
exahype2::dg::tests::KernelsTest::testTheTwoDifferencesAddUpToTheFluxJump() {
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
    0
  );

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      const int entry = faceNode * Unknowns + unknown;
      validateNumericalEqualsWithParams1(
        leftFlux[entry] + rightFlux[entry],
        (1.5 * faceNode - unknown) - (0.5 * faceNode + unknown),
        entry
      );
    }
  }
}

/**
 * The nonconservative jump term enters both stored differences with the same sign.
 */
void
exahype2::dg::tests::KernelsTest::testNonconservativeProductEntersBothSidesAlike() {
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
    0
  );

  // The stub returns the state jump, which the kernel adds in halves to both differences, so
  // the two add up to that jump.
  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      const double stateJump = rightState[faceNode * Data + unknown] - leftState[faceNode * Data + unknown];
      validateNumericalEquals(
        leftFlux[faceNode * Unknowns + unknown] + rightFlux[faceNode * Unknowns + unknown],
        stateJump
      );
    }
  }
}

/**
 * The buffer the nonconservative product reads carries the jump of every auxiliary variable next to the unknowns.
 */
void
exahype2::dg::tests::KernelsTest::testNonconservativeProductReadsTheAuxiliaryJump() {
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
    // The unknowns carry no jump, so only the auxiliary variable can reach the flux.
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      leftState[firstEntry + unknown]  = 1.0;
      rightState[firstEntry + unknown] = 1.0;
    }
    leftState[firstEntry + Unknowns]  = 2.0 + faceNode;
    rightState[firstEntry + Unknowns] = 5.0 + 2.0 * faceNode;
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

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
    0
  );

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    const double auxiliaryJump = rightState[faceNode * Data + Unknowns] - leftState[faceNode * Data + Unknowns];
    for (int unknown = 0; unknown < Unknowns; unknown++) {
      const double halfProduct = 0.5 * (1.0 + unknown) * auxiliaryJump;
      validateNumericalEqualsWithParams2(leftFlux[faceNode * Unknowns + unknown], halfProduct, faceNode, unknown);
      validateNumericalEqualsWithParams2(rightFlux[faceNode * Unknowns + unknown], halfProduct, faceNode, unknown);
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testLinearFormBuildsTheFluxFromTheStateJump() {
  constexpr int Unknowns = LinearFluxSolver::NumberOfUnknowns;
  constexpr int Data     = Unknowns + LinearFluxSolver::NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[RiemannFaceNodes];
  fillNormalisedWeights(faceQuadratureWeights);

  // Nonzero incoming values prove the linear form starts from a clean slate.
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
    0
  );

  // Half the jump is one, the stub flux returns it unchanged, and the wave
  // speed is the averaged first entry, which is three.
  constexpr double ExpectedFlux          = 1.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}

void
exahype2::dg::tests::KernelsTest::testLinearFormAddsTheNonconservativeProduct() {
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
    0
  );

  // Flux and nonconservative product both return half the jump, so the central
  // part doubles while the dissipation stays as it was.
  constexpr double ExpectedCentralFlux   = 2.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}

void
exahype2::dg::tests::KernelsTest::testLinearFormAppliesTheMaterialParameterMatrix() {
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
    0
  );

  // The material parameter matrix doubles the flux the stub produced.
  constexpr double ExpectedCentralFlux   = 2.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < RiemannFaceNodes * Unknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}

/**
 * An interior face holds the Rusanov flux minus the lower own flux and the upper own flux minus the Rusanov flux.
 */
void
exahype2::dg::tests::KernelsTest::testAnInteriorFaceHoldsTheDifferencesToItsOwnFluxes() {
  Face face;
  fill(face);

  solveFace<FaceSolver>(face, 0, -1);

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    const double lowerOwnFlux  = LowerOwnFlux + faceNode;
    const double numericalFlux = 0.5 * (lowerOwnFlux + UpperOwnFlux + WaveSpeed * ((2.0 + faceNode) - 1.0));
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], numericalFlux - lowerOwnFlux, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FaceFluxSize + faceNode], UpperOwnFlux - numericalFlux, faceNode);
  }
}

/**
 * A boundary face takes the boundary state into its outer half and its inner half holds the difference to its own
 * flux.
 */
void
exahype2::dg::tests::KernelsTest::testABoundaryFaceTakesTheBoundaryState() {
  Face face;
  fill(face);

  // The face is the lower face of the cell inside, so the outer half is the lower one.
  solveFace<FaceSolver>(face, 0, 0);

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    validateNumericalEqualsWithParams1(face.faceState[faceNode], OutsideValue, faceNode);
    validateNumericalEqualsWithParams1(face.faceState[FaceStateSize + faceNode], 1.0, faceNode);
    // The solver has no flux, so the outer flux is zero and the Rusanov flux is the jump term alone.
    const double numericalFlux = 0.5 * (0.0 + UpperOwnFlux + WaveSpeed * (OutsideValue - 1.0));
    validateNumericalEqualsWithParams1(face.faceFlux[FaceFluxSize + faceNode], UpperOwnFlux - numericalFlux, faceNode);
  }
}

/**
 * The solver's own Riemann solver runs once with the stamp, the step size, the normal, the
 * boundary flag and the index of the face it writes, and its fluxes become the differences to the
 * own fluxes.
 */
void
exahype2::dg::tests::KernelsTest::testTheSolversRiemannSolverReceivesTheFace() {
  Face face;
  fill(face);

  constexpr int UpperFaceOfTheFirstAxis = DIMENSIONS;
  solveFace<FaceSolverWithRiemannSolver>(face, 0, UpperFaceOfTheFirstAxis);

  const double increment = encodeFaceArguments(TimeStamp, TimeStepSize, 0, true, UpperFaceOfTheFirstAxis);
  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], increment, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FaceFluxSize + faceNode], -increment, faceNode);
  }
}

/**
 * The linear Riemann solver of a system without flux writes the dissipation of the state jump
 * with opposite signs into the two flux halves.
 */
void
exahype2::dg::tests::KernelsTest::testTheLinearRiemannSolverWritesOppositeDissipations() {
  Face face;
  fill(face);

  solveFace<LinearFaceSolver>(face, 0, -1);

  for (int faceNode = 0; faceNode < RiemannFaceNodes; faceNode++) {
    const double dissipation = WaveSpeed * 0.5 * (1.0 - (2.0 + faceNode));
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], -dissipation, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FaceFluxSize + faceNode], dissipation, faceNode);
  }
}

void
exahype2::dg::tests::KernelsTest::testZeroFluxLeavesUpdateUntouched() {
  const Tables tables                                 = getTables();
  const double faceFlux[FaceNodes * NumberOfUnknowns] = {};

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int orientation = 0; orientation < 2; orientation++) {
      double cellUpdate[CellNodes * NumberOfUnknowns];
      double valuesBeforeTheIntegral[CellNodes * NumberOfUnknowns];
      for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
        cellUpdate[entry]              = 1.0 / 3.0 + entry;
        valuesBeforeTheIntegral[entry] = cellUpdate[entry];
      }

      addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, direction, orientation, 2.0, 0.5);

      for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
        validateEqualsWithParams3(cellUpdate[entry], valuesBeforeTheIntegral[entry], direction, orientation, entry);
      }
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testEachCellNodeReceivesItsOwnFaceNode() {
  Tables tables = getTables();
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    tables.inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    // Only one face node carries a flux, so the cell nodes that pick it up are
    // exactly the ones the lifting is supposed to reach.
    for (int markedFaceNode = 0; markedFaceNode < FaceNodes; markedFaceNode++) {
      double    faceFlux[FaceNodes * NumberOfUnknowns] = {};
      const int markedEntry                            = markedFaceNode * NumberOfUnknowns;
      faceFlux[markedEntry]                            = 1.0;

      double cellUpdate[CellNodes * NumberOfUnknowns] = {};

      addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, direction, 0, 1.0, 1.0);

      double expectedSum = 0.0;
      for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
        const int    cellNode   = getCellNodeOf(direction, markedFaceNode, normalIndex);
        const int    firstEntry = cellNode * NumberOfUnknowns;
        const double expectedValue =
          -tables.faceQuadratureWeights[markedFaceNode] * tables.leftFaceCoefficients[normalIndex];
        validateNumericalEquals(cellUpdate[firstEntry], expectedValue);
        validateNumericalEquals(cellUpdate[firstEntry + 1], 0.0);
        expectedSum += expectedValue;
      }

      double sumOfUpdates = 0.0;
      for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
        sumOfUpdates += cellUpdate[entry];
      }
      validateNumericalEquals(sumOfUpdates, expectedSum);
    }
  }
}

/**
 * The update holds NumberOfUnknowns entries per cell node, so every unknown of every cell node
 * receives its own face value and the entries behind the last cell node stay as the caller left
 * them.
 */
void
exahype2::dg::tests::KernelsTest::testUpdateStridesByTheUnknowns() {
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

  addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, Direction, Orientation, 1.0, 1.0);

  for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
    for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
      const int cellNode = getCellNodeOf(Direction, faceNode, normalIndex);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode]
          * tables.faceQuadratureWeights[faceNode] * tables.rightFaceCoefficients[normalIndex]
          * faceFlux[faceNode * NumberOfUnknowns + unknown];
        validateNumericalEqualsWithParams2(
          cellUpdate[cellNode * NumberOfUnknowns + unknown],
          expectedValue,
          cellNode,
          unknown
        );
      }
    }
  }

  for (int entry = 0; entry < GuardEntries; entry++) {
    validateNumericalEqualsWithParams1(cellUpdate[CellNodes * NumberOfUnknowns + entry], GuardValue, entry);
  }
}

/**
 * The lower and the upper face subtract their flux differences with the same sign.
 */
void
exahype2::dg::tests::KernelsTest::testBothOrientationsEnterWithTheSameSign() {
  Tables tables = getTables();
  // Both faces use the same coefficient table here, so the orientation is the only difference left.
  for (int index = 0; index < NodesPerAxis; index++) {
    tables.rightFaceCoefficients[index] = tables.leftFaceCoefficients[index];
  }

  double faceFlux[FaceNodes * NumberOfUnknowns];
  for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
    faceFlux[entry] = 1.0 + entry;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double lowerFace[CellNodes * NumberOfUnknowns] = {};
    double upperFace[CellNodes * NumberOfUnknowns] = {};

    for (int orientation = 0; orientation < 2; orientation++) {
      double* const update = (orientation == 0) ? lowerFace : upperFace;
      addFaceIntegralToEveryNode(update, faceFlux, tables, direction, orientation, 1.0, 1.0);
    }

    for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
      validateEqualsWithParams2(lowerFace[entry], upperFace[entry], direction, entry);
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testWeightsAndCoefficientsEnterAsAProduct() {
  constexpr int    Direction       = 0;
  constexpr int    Orientation     = 1;
  constexpr double InverseCellSize = 3.0;
  constexpr double StepSize        = 0.25;

  const Tables tables = getTables();
  double       faceFlux[FaceNodes * NumberOfUnknowns];
  for (int entry = 0; entry < FaceNodes * NumberOfUnknowns; entry++) {
    faceFlux[entry] = 0.5 + 0.25 * entry;
  }

  double cellUpdate[CellNodes * NumberOfUnknowns] = {};

  addFaceIntegralToEveryNode(cellUpdate, faceFlux, tables, Direction, Orientation, InverseCellSize, StepSize);

  for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
    const double scaledWeight = InverseCellSize * StepSize * tables.faceQuadratureWeights[faceNode];
    for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
      const int cellNode = getCellNodeOf(Direction, faceNode, normalIndex);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode] * scaledWeight
          * tables.rightFaceCoefficients[normalIndex] * faceFlux[faceNode * NumberOfUnknowns + unknown];
        validateNumericalEquals(cellUpdate[cellNode * NumberOfUnknowns + unknown], expectedValue);
      }
    }
  }
}

/**
 * addFaceIntegral() writes the NumberOfUnknowns updates of the node it receives and nothing behind them.
 */
void
exahype2::dg::tests::KernelsTest::testANodeReceivesOnlyItsOwnUpdate() {
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
  double everyNodeUpdate[CellNodes * NumberOfUnknowns] = {};
  addFaceIntegralToEveryNode(everyNodeUpdate, faceFlux, tables, Direction, Orientation, 1.0, 1.0);

  double nodeUpdate[NumberOfUnknowns + GuardEntries] = {};
  for (int entry = NumberOfUnknowns; entry < NumberOfUnknowns + GuardEntries; entry++) {
    nodeUpdate[entry] = GuardValue;
  }
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

  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    validateEqualsWithParams1(nodeUpdate[unknown], everyNodeUpdate[CellNode * NumberOfUnknowns + unknown], unknown);
  }
  for (int entry = NumberOfUnknowns; entry < NumberOfUnknowns + GuardEntries; entry++) {
    validateEqualsWithParams1(nodeUpdate[entry], GuardValue, entry);
  }
}

void
exahype2::dg::tests::KernelsTest::testAnisotropicAccumulation() {
  // Both the eigenvalues and the cell extents differ per axis, so a mix-up of the
  // axes changes the scaled maximum. The largest entry of each table lies on one
  // of the first two axes, so the physical maxima hold in two and in three dimensions.
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
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);

  statistics.accumulate(cellSize, [&](const int dimension) { return firstPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 5.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, firstScaledEigenvalue);

  statistics.accumulate(cellSize, [&](const int dimension) { return secondPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 7.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, secondScaledEigenvalue);

  exahype2::dg::MaxEigenvalueStatistics stationaryStatistics;
  stationaryStatistics.accumulate(cellSize, []([[maybe_unused]] const int dimension) { return 0.0; });
  validateNumericalEquals(stationaryStatistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(stationaryStatistics.maxScaledEigenvalue, 0.0);
}

void
exahype2::dg::tests::KernelsTest::testInvalidEigenvalueAccumulation() {
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::MaxEigenvalueStatistics negativeStatistics;
  negativeStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? -2.0 : 5.0; });
  validateEquals(negativeStatistics.maxPhysicalEigenvalue, -2.0);
  validateEquals(negativeStatistics.maxScaledEigenvalue, -2.0);

  constexpr auto                        PositiveInfinity = std::numeric_limits<double>::infinity();
  exahype2::dg::MaxEigenvalueStatistics infiniteStatistics;
  infiniteStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? PositiveInfinity : 5.0; });
  validateEquals(infiniteStatistics.maxPhysicalEigenvalue, PositiveInfinity);
  validateEquals(infiniteStatistics.maxScaledEigenvalue, PositiveInfinity);
}

void
exahype2::dg::tests::KernelsTest::testReduceMaxEigenvaluesOverTheCellNodes() {
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

  const auto statistics = exahype2::dg::reduceMaxEigenvalues<EigenvalueStubSolver>(
    cellState,
    quadratureNodes,
    cellCentre,
    cellSize,
    0.0,
    0.0
  );

  // The last node carries the largest state and the largest first coordinate,
  // so both maxima come from that node, with every axis scaled by its own extent.
  const double lastPosition   = cellCentre(0) - 0.5 * cellSize(0) + quadratureNodes[1] * cellSize(0);
  const double lastState      = 0.5 * (EigenvalueCellNodes - 1);
  double       expectedScaled = 0.0;
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    expectedScaled += (lastState + direction + 0.25 * lastPosition) / cellSize(direction);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, lastState + (DIMENSIONS - 1) + 0.25 * lastPosition);
  validateNumericalEquals(statistics.maxScaledEigenvalue, expectedScaled);
}

/**
 * The combination takes the larger physical and scaled maxima and keeps a negative entry.
 */
void
exahype2::dg::tests::KernelsTest::testTheReductionKeepsTheLargerAndAnInvalidObservation() {
  const auto identity = exahype2::dg::MaxEigenvalueStatistics::identity();
  validateEquals(identity.maxPhysicalEigenvalue, 0.0);
  validateEquals(identity.maxScaledEigenvalue, 0.0);

  exahype2::dg::MaxEigenvalueStatistics left;
  left.maxPhysicalEigenvalue = 2.0;
  left.maxScaledEigenvalue   = 8.0;
  exahype2::dg::MaxEigenvalueStatistics right;
  right.maxPhysicalEigenvalue = 3.0;
  right.maxScaledEigenvalue   = 5.0;
  const auto combined         = exahype2::dg::MaxEigenvalueStatistics::combine(left, right);
  validateEquals(combined.maxPhysicalEigenvalue, 3.0);
  validateEquals(combined.maxScaledEigenvalue, 8.0);

  right.maxPhysicalEigenvalue = -1.0;
  const auto invalid          = exahype2::dg::MaxEigenvalueStatistics::combine(left, right);
  validateEquals(invalid.maxPhysicalEigenvalue, -1.0);
  validateEquals(invalid.maxScaledEigenvalue, 8.0);
}

/**
 * The state interpolation of a level jump writes the coarse-side half of every fine face and leaves the other half.
 */
void
exahype2::dg::tests::KernelsTest::testTheStateInterpolationFillsTheCoarseSideOfTheFineFaces() {
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
  exahype2::dg::tests::fillTransferConstant<TransferSetup::FaceNodes>(
    faces.getChunk(0) + CoarseSide * StateElementsPerFace
  );

  const exahype2::dg::tests::TransferOperators operators;
  exahype2::dg::InterpolateFaceStateHalves<TransferLayout, double>{faces.getView(), operators.prolongation.data()}(
    levelJump
  );

  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    const double* const fineFace = faces.getChunk(1 + subface);
    for (int node = 0; node < TransferSetup::FaceNodes; node++) {
      for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
        const int element = node * TransferSetup::ValuesPerNode + value;
        validateEqualsWithParams2(
          fineFace[CoarseSide * StateElementsPerFace + element],
          exahype2::dg::tests::getTransferConstantOf(value),
          subface,
          element
        );
        validateEqualsWithParams2(
          fineFace[(1 - CoarseSide) * StateElementsPerFace + element],
          Marker,
          subface,
          element
        );
      }
    }
  }
}

/**
 * The state restriction of a level jump writes the fine-side half of the coarse face and leaves its coarse-side half.
 */
void
exahype2::dg::tests::KernelsTest::testTheStateRestrictionFillsTheFineSideOfTheCoarseFace() {
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
    exahype2::dg::tests::fillTransferConstant<TransferSetup::FaceNodes>(
      faces.getChunk(1 + subface) + FineSide * StateElementsPerFace
    );
  }

  const exahype2::dg::tests::TransferOperators operators;
  exahype2::dg::RestrictStateHalves<TransferLayout, double>{faces.getView(), operators.restriction.data()}(levelJump);

  const double* const coarseFace = faces.getChunk(0);
  for (int node = 0; node < TransferSetup::FaceNodes; node++) {
    for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
      const int element = node * TransferSetup::ValuesPerNode + value;
      validateNumericalEqualsWithParams1(
        coarseFace[FineSide * StateElementsPerFace + element],
        exahype2::dg::tests::getTransferConstantOf(value),
        element
      );
      validateEqualsWithParams1(coarseFace[CoarseSide * StateElementsPerFace + element], Marker, element);
    }
  }
}

/**
 * A cell that survives the regrid keeps every value of its old slot.
 */
void
exahype2::dg::tests::KernelsTest::testCopyKeepsEveryValue() {
  CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
  pool.reserve(1);
  exahype2::dg::tests::fillLinearTransferProfile<TransferSetup::CellNodes>(pool.getChunk(0), DIMENSIONS, -1);

  const exahype2::dg::tests::TransferOperators operators;
  double                                       target[ValuesPerTransferCell];
  getRegridOf(pool, operators).copy(0, target);

  for (int value = 0; value < ValuesPerTransferCell; value++) {
    validateEqualsWithParams1(target[value], pool.getChunk(0)[value], value);
  }
}

/**
 * Every new child takes the state of its parent at its own node positions.
 */
void
exahype2::dg::tests::KernelsTest::testInterpolationFromTheParentReproducesALinearState() {
  CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
  pool.reserve(1);
  exahype2::dg::tests::fillLinearTransferProfile<TransferSetup::CellNodes>(pool.getChunk(0), DIMENSIONS, -1);

  const exahype2::dg::tests::TransferOperators operators;
  for (int child = 0; child < TransferSetup::ChildrenPerCell; child++) {
    double target[ValuesPerTransferCell];
    getRegridOf(pool, operators).interpolateFromParent(0, target, child);

    for (int node = 0; node < TransferSetup::CellNodes; node++) {
      double coordinates[DIMENSIONS];
      exahype2::dg::tests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, child);
      for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
        validateNumericalEqualsWithParams3(
          target[node * TransferSetup::ValuesPerNode + value],
          exahype2::dg::tests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
          child,
          node,
          value
        );
      }
    }
  }
}

/**
 * A coarsened parent takes the state its children represent at its own node positions.
 */
void
exahype2::dg::tests::KernelsTest::testRestrictionFromTheChildrenReproducesALinearState() {
  CellPool pool(ValuesPerTransferCell, tarch::MemoryLocation::Heap);
  pool.reserve(TransferSetup::ChildrenPerCell);
  int childSlots[TransferSetup::ChildrenPerCell];
  for (int child = 0; child < TransferSetup::ChildrenPerCell; child++) {
    exahype2::dg::tests::fillLinearTransferProfile<TransferSetup::CellNodes>(pool.getChunk(child), DIMENSIONS, child);
    childSlots[child] = child;
  }

  const exahype2::dg::tests::TransferOperators operators;
  double                                       target[ValuesPerTransferCell];
  getRegridOf(pool, operators).restrictFromChildren(childSlots, target);

  for (int node = 0; node < TransferSetup::CellNodes; node++) {
    double coordinates[DIMENSIONS];
    exahype2::dg::tests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, -1);
    for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
      validateNumericalEqualsWithParams2(
        target[node * TransferSetup::ValuesPerNode + value],
        exahype2::dg::tests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
        node,
        value
      );
    }
  }
}

void
exahype2::dg::tests::KernelsTest::testExtrapolateStateToFacesOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::getRamp(Setup::AllCellStatesCardinality, 0.4, 0.013);
  const std::vector<double> faceStates(geometry.getNumberOfFaceStateEntries(), 0.0);

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

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testExtrapolateFluxToFacesOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::getRamp(Setup::AllCellStatesCardinality, 0.3, 0.017);
  const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

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
    exahype2::dg::extrapolateFluxToFaces<Stub>(
      facesOfCell,
      cellStates.data() + cellStateOffset,
      operators.leftFaceCoefficients.data(),
      operators.rightFaceCoefficients.data(),
      operators.quadratureNodes.data(),
      operators.derivativeOperator.data(),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
  }

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testEvaluateFluxOnFaceOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double>                  faceStates =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.7, 0.009);
  const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

  const std::vector<double> deviceResult = getFluxesOnFacesOf(operators, geometry, faceStates, faceFluxes);

  std::vector<double> hostResult = faceFluxes;
  for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
    for (int side = 0; side < 2; side++) {
      const int   faceNumber       = geometry.workFaceNumbers[face];
      const int   cell             = geometry.workCellIndices[face];
      const int   sideFluxOffset   = side * Setup::FaceSideFluxCardinality;
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::evaluateFluxOnFace<Stub>(
        faceFlux + sideFluxOffset,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        side,
        operators.quadratureNodes.data(),
        exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  }

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testApplyBoundaryConditionsOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double>                  faceStates =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.2, 0.011);

  const std::vector<double> deviceResult = getBoundaryConditionsOf(operators, geometry, faceStates);

  std::vector<double> hostResult = faceStates;
  for (int face = 0; face < geometry.getNumberOfWorkFaces(); face++) {
    const int faceNumber       = geometry.workFaceNumbers[face];
    const int cell             = geometry.workCellIndices[face];
    const int coordinateOffset = cell * DIMENSIONS;
    exahype2::dg::applyBoundaryConditions<Stub>(
      hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality,
      operators.quadratureNodes.data(),
      exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      faceNumber
    );
  }

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testSolveRiemannProblemOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double>                  faceStates =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.6, 0.007);
  // The nonlinear solver reads the extrapolated fluxes before it overwrites them.
  const std::vector<double> faceFluxes =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceFluxEntries(), -0.3, 0.019);

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
      exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize,
      faceNumber % DIMENSIONS
    );
  }

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testSolveLinearRiemannProblemOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double>                  faceStates =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceStateEntries(), 0.9, 0.005);
  const std::vector<double> faceFluxes(geometry.getNumberOfFaceFluxEntries(), 0.0);

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
      exahype2::dg::tests::getFaceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize,
      faceNumber % DIMENSIONS
    );
  }

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testAddFaceIntegralOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  // The lift accumulates, so both paths have to start from the same estimates.
  const std::vector<double> rightHandSideEstimates =
    exahype2::dg::tests::getRamp(Setup::AllRightHandSidesCardinality, 0.1, 0.003);
  const std::vector<double> faceFluxes =
    exahype2::dg::tests::getRamp(geometry.getNumberOfFaceFluxEntries(), -0.2, 0.009);

  const std::vector<double> deviceResult = getFaceIntegralsOf(operators, geometry, rightHandSideEstimates, faceFluxes);

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

  validateNumericalBuffersEquals(deviceResult, hostResult);
}

void
exahype2::dg::tests::KernelsTest::testReduceMaxEigenvaluesOnTheDevice() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::getRamp(Setup::AllCellStatesCardinality, 0.25, 0.031);

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
        Setup::TimeStepSize
      )
    );
  }

  // A maximum ignores the order in which the tree combines the cells, so the
  // parallel reduction has to land on exactly the serial value.
  validateNumericalEquals(deviceResult.maxScaledEigenvalue, hostResult.maxScaledEigenvalue);
  validateNumericalEquals(deviceResult.maxPhysicalEigenvalue, hostResult.maxPhysicalEigenvalue);
  validate(hostResult.maxPhysicalEigenvalue > 0.0);
}
