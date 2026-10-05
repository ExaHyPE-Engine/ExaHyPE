// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaterialParameters.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/PointSource.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename ComputePrecision>
  class LinearPredictor;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class LinearPredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The Cauchy-Kovalewski space-time predictor of a linear system. A term that Derived does not
 * declare has no buffer and no code. Every buffer uses the padded strides of
 * exahype2::dg::PaddedLayout.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::ader::LinearPredictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tables     = exahype2::dg::BasisTables<ComputePrecision>;

  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;
  static constexpr bool EvaluateSource             = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateMaterialParameters = exahype2::terms::HasMaterialParameters<Derived, ComputePrecision>;
  static constexpr bool EvaluatePointSources       = exahype2::terms::HasPointSource<Derived, ComputePrecision>;
  static constexpr bool AccumulatesFlux            = EvaluateFlux or EvaluateNonconservativeProduct;
  static constexpr bool AccumulatesSource          = EvaluateSource;

  /**
   * The extents and offsets of the scratch block of predict(). After predict(), the flux block
   * stores the spatial terms of the time-averaged state along every axis: The flux derivative, the
   * nonconservative product and the point sources.
   */
  struct Scratch final {
    static constexpr int StateSize             = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int TimeAveragedStateSize = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int FluxSize = DIMENSIONS * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
    static constexpr int StateGradientSize =
      EvaluateNonconservativeProduct ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerAxis : 0;
    static constexpr int PointSourceSize = EvaluatePointSources
      ? (CellLayout::NodesPerAxis + 1) * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns
      : 0;

    static constexpr int StateOffset             = 0;
    static constexpr int TimeAveragedStateOffset = StateOffset + StateSize;
    static constexpr int FluxOffset              = TimeAveragedStateOffset + TimeAveragedStateSize;
    static constexpr int StateGradientOffset     = FluxOffset + FluxSize;
    static constexpr int PointSourceOffset       = StateGradientOffset + StateGradientSize;
    static constexpr int Size                    = PointSourceOffset + PointSourceSize;
  };

  template <int NumberOfPointSources>
  static INLINE_METHOD GPU_CALLABLE_METHOD int selectPointSources(
    int (&pointSourceIndices)[NumberOfPointSources],
    const double* NOALIAS                        pointSourceLocations,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void predict(
    ComputePrecision* const NOALIAS              faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    ComputePrecision* NOALIAS                    scratch,
    const int* NOALIAS                           pointSourceIndices,
    int                                          numberOfPointSources,
    const double* NOALIAS                        pointSourceLocations,
    const Tables&                                tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

private:
  friend class exahype2::dg::ader::tests::LinearPredictorTest;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialiseState(
    ComputePrecision* NOALIAS       state,
    const StoragePrecision* NOALIAS cellState
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double evaluateLagrangeBasis(
    const ComputePrecision* NOALIAS quadraturePoints,
    int                             index,
    double                          position
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluatePointSources(
    ComputePrecision* NOALIAS                              pointSourceValues,
    const StoragePrecision* NOALIAS                        cellState,
    const int* NOALIAS                                     pointSourceIndices,
    int                                                    numberOfPointSources,
    const double* NOALIAS                                  pointSourceLocations,
    const ComputePrecision* NOALIAS                        quadraturePoints,
    const ComputePrecision* NOALIAS                        inverseCellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToState(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights,
    double                          timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void seedTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearFluxes(ComputePrecision* NOALIAS flux);

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateFlux(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addNonconservativeProduct(
    ComputePrecision* NOALIAS                              flux,
    ComputePrecision* NOALIAS                              stateGradient,
    const ComputePrecision* NOALIAS                        state,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void applyMaterialParameterMatrix(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void overwriteStateWithLinearSource(
    ComputePrecision* NOALIAS                    state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addFluxesToTimeDerivative(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS flux
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourceDerivativeToState(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS derivativeCoefficients
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state,
    ComputePrecision                timeAverageFactor
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToTimeAveragedFlux(
    ComputePrecision* NOALIAS       timeAveragedFlux,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/LinearPredictor.cpph"
