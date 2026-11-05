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
  template <typename Derived, typename ComputePrecision>
  class NonlinearPredictor;
} // namespace exahype2::dg::ader

TARGET_DECLARE
/**
 * The space-time predictor of one cell of a nonlinear ADER-DG solver Derived. predict() iterates
 * the Picard system of the space-time state, writes the time-averaged state and flux of the cell
 * onto the halves of its faces and returns the number of Picard sweeps or an outcome of
 * PicardOutcome.h. The threads of a team run several cells together, one group of threads per cell.
 * Every node of a cell counts the squared changes of its own values in integer units of the squared
 * tolerance and the first thread of the cell adds the node counts. The residual of a sweep is the
 * same for every number of threads per cell and for every order of the nodes and of the unknowns.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::ader::NonlinearPredictor final {
public:
  using CellLayout        = exahype2::dg::PaddedLayout<Derived>;
  using Tables            = exahype2::dg::BasisTables<ComputePrecision>;
  using DerivativeWeights = exahype2::dg::DerivativeWeightsOfNodePairs<CellLayout::NodesPerAxis, ComputePrecision>;

  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, ComputePrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;
  static constexpr bool EvaluateSource          = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateDiffusiveSource = exahype2::terms::HasDiffusiveSourceTerm<Derived, ComputePrecision>;
  static constexpr bool AccumulatesFlux         = EvaluateFlux;
  /**
   * False when Derived sets DifferentiatesTimeAveragedFlux to false because its caller reads only
   * the faces and the source of the prediction.
   */
  static constexpr bool DifferentiatesTimeAveragedFlux = EvaluateFlux and [] {
    if constexpr (requires { Derived::DifferentiatesTimeAveragedFlux; }) {
      return Derived::DifferentiatesTimeAveragedFlux;
    } else {
      return true;
    }
  }();
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
    static constexpr int NodeResidualSize = exahype2::dg::getPaddedCountOf(CellLayout::NodesPerCell);

    static constexpr int SpaceTimeStateOffset     = 0;
    static constexpr int TimeAveragedStateOffset  = SpaceTimeStateOffset;
    static constexpr int RightHandSideOffset      = SpaceTimeStateOffset + SpaceTimeStateSize;
    static constexpr int FluxOffset               = RightHandSideOffset + RightHandSideSize;
    static constexpr int FirstFluxOffset          = FluxOffset + FluxSize;
    static constexpr int TimeAveragedFluxOffset   = FirstFluxOffset + FirstFluxSize;
    static constexpr int FirstSourceOffset        = TimeAveragedFluxOffset + TimeAveragedFluxSize;
    static constexpr int TimeAveragedSourceOffset = FirstSourceOffset + FirstSourceSize;
    static constexpr int StateGradientOffset      = TimeAveragedSourceOffset + TimeAveragedSourceSize;
    static constexpr int NodeResidualOffset       = StateGradientOffset + StateGradientSize;
    static constexpr int Size                     = NodeResidualOffset + NodeResidualSize;
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
    const typename Derived::RuntimeParameters&   parameters,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getTeamScratchBytes(int teamThreads);

private:
  static constexpr int ValuesPerNodeResidual = CellLayout::NodesPerAxis * CellLayout::NumberOfUnknowns;

  /**
   * The units of one squared tolerance in the node residuals. The units of the values of one node
   * add up to at most 2^(digits - 1) and then add exactly in ComputePrecision.
   */
  static constexpr double ResidualUnitsOfTolerance = []() {
    constexpr double LargestExactSum =
      static_cast<double>(std::uint64_t{1} << (std::numeric_limits<ComputePrecision>::digits - 1));
    double units = LargestExactSum;
    while (units * ValuesPerNodeResidual > LargestExactSum) {
      units *= 0.5;
    }
    return units;
  }();

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getResidualOfCell(
    const ComputePrecision* NOALIAS nodeResiduals,
    const TeamWorkers&              workers
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double sumNodeResiduals(
    const ComputePrecision* NOALIAS nodeResiduals
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getLargestOfCell(
    double             value,
    const TeamWorkers& workers
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getSquaredTolerance(
    const StoragePrecision* NOALIAS cellState,
    double                          picardTolerance,
    const TeamWorkers&              workers = TeamWorkers{}
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
    const DerivativeWeights&                               derivativeWeights,
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
    const typename Derived::RuntimeParameters&   parameters,
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

  static INLINE_METHOD GPU_CALLABLE_METHOD void differentiateFlux(
    ComputePrecision*                                      fluxDerivative,
    const ComputePrecision*                                flux,
    const DerivativeWeights&                               stiffnessWeights,
    const ComputePrecision* NOALIAS                        cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    double                                                 timeStepSize,
    int                                                    timeNode,
    const TeamWorkers&                                     workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractFluxDivergence(
    ComputePrecision* NOALIAS       rightHandSide,
    const ComputePrecision* NOALIAS fluxDerivative,
    int                             timeNode,
    const TeamWorkers&              workers = TeamWorkers{}
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
    const typename Derived::RuntimeParameters&   parameters,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void solvePicardSystem(
    ComputePrecision* NOALIAS       spaceTimeState,
    ComputePrecision* NOALIAS       nodeResiduals,
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
