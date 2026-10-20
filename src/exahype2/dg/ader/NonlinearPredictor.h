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

  template <typename Derived, typename PredictorPrecision>
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
 * The space-time predictor of one cell of a nonlinear ADER-DG solver Derived. predict() iterates
 * the Picard system of the space-time state, writes the time-averaged state and flux of the cell
 * onto the halves of its faces and returns the number of Picard sweeps or an outcome of
 * PicardOutcome.h. The threads of a team run several cells together, one group of threads per cell.
 */
template <typename Derived, typename PredictorPrecision>
class exahype2::dg::ader::NonlinearPredictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tables     = exahype2::dg::BasisTables<PredictorPrecision>;

  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, PredictorPrecision>;
  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, PredictorPrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, PredictorPrecision>;
  static constexpr bool EvaluateSource          = exahype2::terms::HasSourceTerm<Derived, PredictorPrecision>;
  static constexpr bool EvaluateDiffusiveSource = exahype2::terms::HasDiffusiveSourceTerm<Derived, PredictorPrecision>;
  static constexpr bool AccumulatesFlux         = EvaluateFlux;
  static constexpr bool AccumulatesSource = EvaluateSource or EvaluateDiffusiveSource or EvaluateNonconservativeProduct;
  static constexpr bool NeedsStateGradient =
    EvaluateNonconservativeProduct or EvaluateDiffusiveFlux or EvaluateDiffusiveSource;

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

  template <typename StoragePrecision, typename CorrectorPrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD int predict(
    CorrectorPrecision* const NOALIAS            faceStates[peano4::FacesPerCell],
    CorrectorPrecision* const NOALIAS            faceFluxes[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    PredictorPrecision* NOALIAS                  scratch,
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
    PredictorPrecision* NOALIAS     spaceTimeState,
    const StoragePrecision* NOALIAS cellState,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void computeSpaceTimeGradient(
    PredictorPrecision* NOALIAS                              stateGradient,
    const PredictorPrecision* NOALIAS                        spaceTimeState,
    const PredictorPrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, PredictorPrecision>& inverseCellSize,
    int                                                      timeNode,
    const TeamWorkers&                                       workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeFlux(
    PredictorPrecision* NOALIAS                  flux,
    const PredictorPrecision* NOALIAS            spaceTimeState,
    const PredictorPrecision* NOALIAS            stateGradient,
    const PredictorPrecision* NOALIAS            quadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void averageFluxOverTime(
    PredictorPrecision* NOALIAS timeAveragedFlux,
    const PredictorPrecision*   flux,
    const PredictorPrecision*   firstFlux,
    PredictorPrecision          weight,
    int                         timeNode,
    const TeamWorkers&          workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearRightHandSide(
    PredictorPrecision* NOALIAS rightHandSide,
    int                         timeNode,
    const TeamWorkers&          workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractFluxDivergence(
    PredictorPrecision* NOALIAS                              rightHandSide,
    const PredictorPrecision* NOALIAS                        flux,
    const PredictorPrecision* NOALIAS                        stiffnessOperatorTransposed,
    const PredictorPrecision* NOALIAS                        cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, PredictorPrecision>& inverseCellSize,
    double                                                   timeStepSize,
    int                                                      timeNode,
    int                                                      axis,
    const TeamWorkers&                                       workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeSource(
    PredictorPrecision* NOALIAS                  rightHandSide,
    PredictorPrecision* NOALIAS                  firstSource,
    PredictorPrecision* NOALIAS                  timeAveragedSource,
    const PredictorPrecision* NOALIAS            spaceTimeState,
    const PredictorPrecision* NOALIAS            stateGradient,
    const PredictorPrecision* NOALIAS            quadraturePoints,
    const PredictorPrecision* NOALIAS            quadratureWeights,
    const PredictorPrecision* NOALIAS            cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD PicardResidual solvePicardSystem(
    PredictorPrecision* NOALIAS       spaceTimeState,
    const PredictorPrecision* NOALIAS rightHandSide,
    const StoragePrecision* NOALIAS   cellState,
    const PredictorPrecision* NOALIAS inversePicardOperator,
    const PredictorPrecision* NOALIAS inverseCellQuadratureWeights,
    double                            squaredTolerance,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void averageStateOverTime(
    PredictorPrecision* NOALIAS       spaceTimeState,
    const PredictorPrecision* NOALIAS quadratureWeights,
    const TeamWorkers&                workers = TeamWorkers{}
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/NonlinearPredictor.cpph"
