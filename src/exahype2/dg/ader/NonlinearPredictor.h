// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/PicardOutcome.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "exahype2/terms/DiffusiveFlux.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  struct PicardResidual;

  template <typename Derived, typename ComputePrecision>
  class NonlinearPredictor;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class NonlinearPredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The squared Picard residual of the nodes one thread solved, in fixed point: highLimb and
 * lowLimb hold the sum of the scaled squares, exceeds records a square at or above the
 * tolerance, and diverged records an iterate outside the divergence bound. add() sums two
 * residuals exactly.
 */
struct exahype2::dg::ader::PicardResidual final {
  std::uint64_t highLimb{0};
  std::uint64_t lowLimb{0};
  bool          exceeds{false};
  bool          diverged{false};

  INLINE_METHOD GPU_CALLABLE_METHOD void add(const PicardResidual& other);
};

/**
 * The space-time predictor of one cell of a nonlinear ADER-DG solver Derived (the generated solver
 * class, whose PDE terms are its static methods and whose DGOrder, NumberOfUnknowns and
 * NumberOfAuxiliaryVariables fix the layout). predict() iterates the Picard system of the
 * space-time state, writes the time-averaged state and flux of the cell onto the halves of its
 * faces and returns the number of Picard sweeps, UnconvergedPicardIteration when the sweep limit
 * comes before the tolerance, or DivergedPicardIteration when an iterate is not finite or leaves
 * the divergence bound. Every quantity is stored with the padded strides of
 * exahype2::dg::PaddedLayout. The threads of a team run several cells together, one group of
 * threads per cell. Every phase of a sweep distributes the nodes or the node lines of a cell over
 * the threads of its group, and a barrier of the team separates two phases of which the second
 * reads what the first wrote.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::ader::NonlinearPredictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tables     = exahype2::dg::BasisTables<ComputePrecision>;

  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, ComputePrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;
  static constexpr bool EvaluateSource          = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateDiffusiveSource = exahype2::terms::HasDiffusiveSourceTerm<Derived, ComputePrecision>;
  static constexpr bool AccumulatesFlux         = EvaluateFlux;
  static constexpr bool AccumulatesSource = EvaluateSource or EvaluateDiffusiveSource or EvaluateNonconservativeProduct;
  static constexpr bool NeedsStateGradient =
    EvaluateNonconservativeProduct or EvaluateDiffusiveFlux or EvaluateDiffusiveSource;

  /**
   * The extents and offsets of the scratch block of predict(). The time-averaged state overwrites
   * the first time slice of the space-time state, and the flux block stores one time slice.
   */
  struct Scratch final {
    static constexpr int SpaceTimeStateSize = CellLayout::PaddedNumberOfData * CellLayout::SpaceTimeNodes;
    static constexpr int RightHandSideSize  = CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes;
    static constexpr int FluxSize =
      EvaluateFlux ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;
    static constexpr int FirstFluxSize        = FluxSize;
    static constexpr int TimeAveragedFluxSize = FluxSize;
    static constexpr int FirstSourceSize =
      AccumulatesSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell : 0;
    static constexpr int TimeAveragedSourceSize = FirstSourceSize;
    static constexpr int StateGradientSize =
      NeedsStateGradient ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;

    static constexpr int SpaceTimeStateOffset     = 0;
    static constexpr int TimeAveragedStateOffset  = SpaceTimeStateOffset;
    static constexpr int RightHandSideOffset      = SpaceTimeStateOffset + SpaceTimeStateSize;
    static constexpr int FluxOffset               = RightHandSideOffset + RightHandSideSize;
    static constexpr int FirstFluxOffset          = FluxOffset + FluxSize;
    static constexpr int TimeAveragedFluxOffset   = FirstFluxOffset + FirstFluxSize;
    static constexpr int FirstSourceOffset        = TimeAveragedFluxOffset + TimeAveragedFluxSize;
    static constexpr int TimeAveragedSourceOffset = FirstSourceOffset + FirstSourceSize;
    static constexpr int StateGradientOffset      = TimeAveragedSourceOffset + TimeAveragedSourceSize;
    static constexpr int Size                     = StateGradientOffset + StateGradientSize;
  };

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD int predict(
    ComputePrecision* const NOALIAS              faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    ComputePrecision* NOALIAS                    scratch,
    const Tables&                                tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double                                       picardTolerance,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getTeamScratchBytes(int teamThreads);
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getSquaredResidual(
    const PicardResidual& residual,
    double                squaredTolerance
  );

private:
  friend class exahype2::dg::ader::tests::NonlinearPredictorTest;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getReservedTeamScratchBytes(int teamThreads);
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD unsigned char* getAlignedTeamScratch(
    const TeamWorkers& workers
  );
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD PicardResidual combineOverGroup(
    const TeamWorkers&    workers,
    const PicardResidual& residual
  );
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isAnyActive(const TeamWorkers& workers, bool active);

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getSquaredTolerance(
    const StoragePrecision* NOALIAS cellState,
    double                          picardTolerance
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialiseState(
    ComputePrecision* NOALIAS       spaceTimeState,
    const StoragePrecision* NOALIAS cellState,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void computeSpaceTimeGradient(
    ComputePrecision* NOALIAS                              stateGradient,
    const ComputePrecision* NOALIAS                        spaceTimeState,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    int                                                    timeNode,
    const TeamWorkers&                                     workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeFlux(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void averageFluxOverTime(
    ComputePrecision* NOALIAS timeAveragedFlux,
    const ComputePrecision*   flux,
    const ComputePrecision*   firstFlux,
    ComputePrecision          weight,
    int                       timeNode,
    const TeamWorkers&        workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearRightHandSide(
    ComputePrecision* NOALIAS rightHandSide,
    int                       timeNode,
    const TeamWorkers&        workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractFluxDivergence(
    ComputePrecision* NOALIAS                              rightHandSide,
    const ComputePrecision* NOALIAS                        flux,
    const ComputePrecision* NOALIAS                        stiffnessOperatorTransposed,
    const ComputePrecision* NOALIAS                        cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    double                                                 timeStepSize,
    int                                                    timeNode,
    int                                                    axis,
    const TeamWorkers&                                     workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeSource(
    ComputePrecision* NOALIAS                    rightHandSide,
    ComputePrecision* NOALIAS                    firstSource,
    ComputePrecision* NOALIAS                    timeAveragedSource,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const ComputePrecision* NOALIAS              quadratureWeights,
    const ComputePrecision* NOALIAS              cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD PicardResidual solvePicardSystem(
    ComputePrecision* NOALIAS       spaceTimeState,
    const ComputePrecision* NOALIAS rightHandSide,
    const StoragePrecision* NOALIAS cellState,
    const ComputePrecision* NOALIAS inversePicardOperator,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights,
    double                          squaredTolerance,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void averageStateOverTime(
    ComputePrecision* NOALIAS       spaceTimeState,
    const ComputePrecision* NOALIAS quadratureWeights,
    const TeamWorkers&              workers = TeamWorkers{}
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/NonlinearPredictor.cpph"
