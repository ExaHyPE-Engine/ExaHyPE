// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/terms/DiffusiveFlux.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename ComputePrecision>
  class NonlinearPredictor;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class NonlinearPredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The space-time predictor of a nonlinear system. predict() runs a Picard iteration that solves
 * the local Cauchy problem of one cell of Derived on the space-time nodes, averages the state, the
 * flux and the source over the time step and writes the time-averaged face states and face fluxes
 * onto the faces of the cell. Derived is the generated solver class: Its DGOrder, NumberOfUnknowns
 * and NumberOfAuxiliaryVariables fix the layout, its static methods are the PDE terms, and a term
 * it does not declare has no buffer and no code. Every quantity is stored with the padded strides
 * of exahype2::dg::PaddedLayout.
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
   * Extents and offsets of the two scratch blocks of predict(). The Picard block holds every
   * space-time quantity, the time-averaged block holds the time integrals. A term Derived does not
   * declare has extent zero.
   */
  struct Scratch final {
    static constexpr int SpaceTimeStateSize = CellLayout::PaddedNumberOfData * CellLayout::SpaceTimeNodes;
    static constexpr int RightHandSideSize  = CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes;
    static constexpr int SpaceTimeFluxSize =
      EvaluateFlux ? CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes * DIMENSIONS : 0;
    static constexpr int SpaceTimeSourceSize =
      AccumulatesSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes : 0;
    static constexpr int StateGradientSize =
      NeedsStateGradient ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;
    static constexpr int UpdatedTimeSliceSize = CellLayout::NodesPerAxis * CellLayout::PaddedNumberOfUnknowns;

    static constexpr int SpaceTimeStateOffset   = 0;
    static constexpr int RightHandSideOffset    = SpaceTimeStateOffset + SpaceTimeStateSize;
    static constexpr int SpaceTimeFluxOffset    = RightHandSideOffset + RightHandSideSize;
    static constexpr int SpaceTimeSourceOffset  = SpaceTimeFluxOffset + SpaceTimeFluxSize;
    static constexpr int StateGradientOffset    = SpaceTimeSourceOffset + SpaceTimeSourceSize;
    static constexpr int UpdatedTimeSliceOffset = StateGradientOffset + StateGradientSize;
    static constexpr int PicardSize             = UpdatedTimeSliceOffset + UpdatedTimeSliceSize;

    static constexpr int TimeAveragedStateSize = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int TimeAveragedFluxSize =
      EvaluateFlux ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell * DIMENSIONS : 0;
    static constexpr int TimeAveragedSourceSize =
      AccumulatesSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell : 0;

    static constexpr int TimeAveragedStateOffset  = 0;
    static constexpr int TimeAveragedFluxOffset   = TimeAveragedStateOffset + TimeAveragedStateSize;
    static constexpr int TimeAveragedSourceOffset = TimeAveragedFluxOffset + TimeAveragedFluxSize;
    static constexpr int TimeAveragedSize         = TimeAveragedSourceOffset + TimeAveragedSourceSize;
  };

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD int predict(
    ComputePrecision* const NOALIAS              faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    ComputePrecision* NOALIAS                    timeAveragedScratch,
    ComputePrecision* NOALIAS                    picardScratch,
    const Tables&                                tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double                                       picardTolerance
  );

private:
  friend class exahype2::dg::ader::tests::NonlinearPredictorTest;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialiseState(
    ComputePrecision* NOALIAS       spaceTimeState,
    const StoragePrecision* NOALIAS cellState
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void computeSpaceTimeGradient(
    ComputePrecision* NOALIAS                              stateGradient,
    const ComputePrecision* NOALIAS                        spaceTimeState,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    int                                                    timeNode
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeFlux(
    ComputePrecision* NOALIAS                    spaceTimeFlux,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearRightHandSide(
    ComputePrecision* NOALIAS rightHandSide,
    int                       timeNode
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractFluxDivergence(
    ComputePrecision* NOALIAS                              rightHandSide,
    const ComputePrecision* NOALIAS                        spaceTimeFlux,
    const ComputePrecision* NOALIAS                        stiffnessOperatorTransposed,
    const ComputePrecision* NOALIAS                        cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    double                                                 timeStepSize,
    int                                                    timeNode
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeSource(
    ComputePrecision* NOALIAS                    spaceTimeSource,
    ComputePrecision* NOALIAS                    rightHandSide,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const ComputePrecision* NOALIAS              quadratureWeights,
    const ComputePrecision* NOALIAS              cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double solvePicardSystem(
    ComputePrecision* NOALIAS       spaceTimeState,
    ComputePrecision* NOALIAS       updatedTimeSlice,
    const ComputePrecision* NOALIAS rightHandSide,
    const StoragePrecision* NOALIAS cellState,
    const ComputePrecision* NOALIAS inversePicardOperator,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void averageOverTime(
    ComputePrecision* NOALIAS       timeAveragedState,
    ComputePrecision* NOALIAS       timeAveragedFlux,
    ComputePrecision* NOALIAS       timeAveragedSource,
    const ComputePrecision* NOALIAS spaceTimeState,
    const ComputePrecision* NOALIAS spaceTimeFlux,
    const ComputePrecision* NOALIAS spaceTimeSource,
    const ComputePrecision* NOALIAS quadratureWeights
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/NonlinearPredictor.cpph"
