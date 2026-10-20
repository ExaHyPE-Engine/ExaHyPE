// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/TeamWorkers.h"
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
  template <typename Derived, typename PredictorPrecision>
  class LinearPredictor;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class LinearPredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The Cauchy-Kovalewski space-time predictor of a linear system. The threads of one cell split its
 * nodes and lines. Every buffer uses the padded strides of exahype2::dg::PaddedLayout.
 */
template <typename Derived, typename PredictorPrecision>
class exahype2::dg::ader::LinearPredictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tables     = exahype2::dg::BasisTables<PredictorPrecision>;

  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, PredictorPrecision>;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, PredictorPrecision>;
  static constexpr bool EvaluateSource             = exahype2::terms::HasSourceTerm<Derived, PredictorPrecision>;
  static constexpr bool EvaluateMaterialParameters = exahype2::terms::HasMaterialParameters<Derived, PredictorPrecision>;
  static constexpr bool EvaluatePointSources       = exahype2::terms::HasPointSource<Derived, PredictorPrecision>;
  static constexpr bool AccumulatesFlux            = EvaluateFlux or EvaluateNonconservativeProduct;
  static constexpr bool AccumulatesSource          = EvaluateSource;

  struct Scratch final {
    static constexpr int StateSize             = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int TimeAveragedStateSize = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int FluxSize        = DIMENSIONS * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
    static constexpr int PointSourceSize = EvaluatePointSources
      ? (CellLayout::NodesPerAxis + 1) * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns
      : 0;

    static constexpr int StateOffset             = 0;
    static constexpr int TimeAveragedStateOffset = StateOffset + StateSize;
    static constexpr int FluxOffset              = TimeAveragedStateOffset + TimeAveragedStateSize;
    static constexpr int PointSourceOffset       = FluxOffset + FluxSize;
    static constexpr int Size                    = PointSourceOffset + PointSourceSize;
  };

  template <int NumberOfPointSources>
  static INLINE_METHOD GPU_CALLABLE_METHOD int selectPointSources(
    int (&pointSourceIndices)[NumberOfPointSources],
    const double* NOALIAS                        pointSourceLocations,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision, typename CorrectorPrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void predict(
    CorrectorPrecision* const NOALIAS            faceStates[peano4::FacesPerCell],
    CorrectorPrecision* const NOALIAS            faceFluxes[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    PredictorPrecision* NOALIAS                  scratch,
    const int* NOALIAS                           pointSourceIndices,
    int                                          numberOfPointSources,
    const double* NOALIAS                        pointSourceLocations,
    const Tables&                                tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

private:
  friend class exahype2::dg::ader::tests::LinearPredictorTest;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialiseState(
    PredictorPrecision* NOALIAS     state,
    const StoragePrecision* NOALIAS cellState,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double evaluateLagrangeBasis(
    const PredictorPrecision* NOALIAS quadraturePoints,
    int                               index,
    double                            position
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluatePointSources(
    PredictorPrecision* NOALIAS                              pointSourceValues,
    const PredictorPrecision* NOALIAS                        state,
    const int* NOALIAS                                       pointSourceIndices,
    int                                                      numberOfPointSources,
    const double* NOALIAS                                    pointSourceLocations,
    const PredictorPrecision* NOALIAS                        quadraturePoints,
    const PredictorPrecision* NOALIAS                        inverseCellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, PredictorPrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&             cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&             cellSize,
    double                                                   timeStamp,
    double                                                   timeStepSize,
    const TeamWorkers&                                       workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToState(
    PredictorPrecision* NOALIAS       state,
    const PredictorPrecision* NOALIAS pointSourceValues,
    const PredictorPrecision* NOALIAS quadratureWeights,
    double                            timeStepSize,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void seedTimeAveragedState(
    PredictorPrecision* NOALIAS       timeAveragedState,
    const PredictorPrecision* NOALIAS state,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearFluxes(
    PredictorPrecision* NOALIAS flux,
    const TeamWorkers&          workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateFlux(
    PredictorPrecision* NOALIAS                  flux,
    const PredictorPrecision* NOALIAS            state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addNonconservativeProduct(
    PredictorPrecision* NOALIAS                              flux,
    const PredictorPrecision* NOALIAS                        state,
    const PredictorPrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, PredictorPrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&             cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&             cellSize,
    double                                                   timeStamp,
    double                                                   timeStepSize,
    const TeamWorkers&                                       workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void applyMaterialParameterMatrix(
    PredictorPrecision* NOALIAS                  flux,
    const PredictorPrecision* NOALIAS            state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void overwriteStateWithLinearSource(
    PredictorPrecision* NOALIAS                  state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addFluxesToTimeDerivative(
    PredictorPrecision* NOALIAS       state,
    const PredictorPrecision* NOALIAS flux,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourceDerivativeToState(
    PredictorPrecision* NOALIAS       state,
    const PredictorPrecision* NOALIAS pointSourceValues,
    const PredictorPrecision* NOALIAS derivativeCoefficients,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedState(
    PredictorPrecision* NOALIAS       timeAveragedState,
    const PredictorPrecision* NOALIAS state,
    PredictorPrecision                timeAverageFactor,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToTimeAveragedFlux(
    PredictorPrecision* NOALIAS       timeAveragedFlux,
    const PredictorPrecision* NOALIAS pointSourceValues,
    const PredictorPrecision* NOALIAS quadratureWeights,
    const TeamWorkers&                workers = TeamWorkers{}
  );

  template<typename CorrectorPrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateToFaces(
    CorrectorPrecision* const NOALIAS faceStates[peano4::FacesPerCell],
    CorrectorPrecision* const NOALIAS faceFluxes[peano4::FacesPerCell],
    const PredictorPrecision* NOALIAS timeAveragedState,
    const Tables&                     tables,
    const TeamWorkers&                workers = TeamWorkers{}
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/LinearPredictor.cpph"
