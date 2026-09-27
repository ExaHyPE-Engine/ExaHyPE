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
 * The space-time predictor of a linear system. predict() runs the Cauchy-Kovalewski recursion,
 * which builds the Taylor coefficients of the state of one cell of Derived one at a time and
 * integrates them over the time step, adds the point sources inside the cell to the coefficients
 * and writes the time-averaged face states and face fluxes onto the faces of the cell. Derived is
 * the generated solver class: Its DGOrder, NumberOfUnknowns and NumberOfAuxiliaryVariables fix the
 * layout, its static methods are the PDE terms, and a term it does not declare has no buffer and no
 * code. Every quantity is stored with the padded strides of exahype2::dg::PaddedLayout.
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
   * Extents and offsets of the one scratch block of predict(). The block holds the current Taylor
   * coefficient of the state, its flux and its source, the time averages of the three, one
   * differentiated line of nodes and the point source samples. A term Derived does not declare
   * has extent zero.
   */
  struct Scratch final {
    static constexpr int StateSize             = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int TimeAveragedStateSize = CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell;
    static constexpr int FluxSize = DIMENSIONS * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
    static constexpr int TimeAveragedFluxSize =
      DIMENSIONS * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
    static constexpr int SourceSize =
      EvaluateSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell : 0;
    static constexpr int TimeAveragedSourceSize =
      EvaluateSource ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell : 0;
    static constexpr int StateGradientSize =
      EvaluateNonconservativeProduct ? CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerAxis : 0;
    static constexpr int PointSourceSize = EvaluatePointSources
      ? (CellLayout::NodesPerAxis + 1) * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns
      : 0;

    static constexpr int StateOffset              = 0;
    static constexpr int TimeAveragedStateOffset  = StateOffset + StateSize;
    static constexpr int FluxOffset               = TimeAveragedStateOffset + TimeAveragedStateSize;
    static constexpr int TimeAveragedFluxOffset   = FluxOffset + FluxSize;
    static constexpr int SourceOffset             = TimeAveragedFluxOffset + TimeAveragedFluxSize;
    static constexpr int TimeAveragedSourceOffset = SourceOffset + SourceSize;
    static constexpr int StateGradientOffset      = TimeAveragedSourceOffset + TimeAveragedSourceSize;
    static constexpr int PointSourceOffset        = StateGradientOffset + StateGradientSize;
    static constexpr int Size                     = PointSourceOffset + PointSourceSize;
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

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearTaylorTerms(
    ComputePrecision* NOALIAS flux,
    ComputePrecision* NOALIAS source
  );

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

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSource(
    ComputePrecision* NOALIAS                    source,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedTerms(
    ComputePrecision* NOALIAS       timeAveragedFlux,
    ComputePrecision* NOALIAS       timeAveragedSource,
    const ComputePrecision* NOALIAS flux,
    const ComputePrecision* NOALIAS source,
    ComputePrecision                timeAverageFactor
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void advanceTimeDerivative(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS flux,
    const ComputePrecision* NOALIAS source
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
