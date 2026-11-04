// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "exahype2/terms/CellPredicate.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaterialParameters.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/PointSource.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename ComputePrecision>
  class LinearPredictor;
} // namespace exahype2::dg::ader

TARGET_DECLARE
/**
 * The Cauchy-Kovalewski space-time predictor of a linear system. The threads of one cell split its
 * nodes and lines. Every buffer uses the padded strides of exahype2::dg::PaddedLayout.
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
  // A Riemann solver of Derived or of a coupling partner reads the face fluxes of the predictor.
  // Only a solver with a cell predicate has a coupling partner.
  // exahype2::dg::solveLinearRiemannProblem() reads the face states only.
  static constexpr bool WritesFaceFluxes =
    exahype2::terms::HasRiemannSolver<Derived, ComputePrecision> or exahype2::terms::HasCellPredicate<Derived>;

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
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

private:
  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialiseState(
    ComputePrecision* NOALIAS       state,
    const StoragePrecision* NOALIAS cellState,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluatePointSources(
    ComputePrecision* NOALIAS                              pointSourceValues,
    const ComputePrecision* NOALIAS                        state,
    const int* NOALIAS                                     pointSourceIndices,
    int                                                    numberOfPointSources,
    const double* NOALIAS                                  pointSourceLocations,
    const ComputePrecision* NOALIAS                        quadraturePoints,
    const ComputePrecision* NOALIAS                        inverseCellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize,
    const TeamWorkers&                                     workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToState(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights,
    double                          timeStepSize,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void seedTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void clearFluxes(
    ComputePrecision* NOALIAS flux,
    const TeamWorkers&        workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void evaluateFlux(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addNonconservativeProduct(
    ComputePrecision* NOALIAS                              flux,
    const ComputePrecision* NOALIAS                        state,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize,
    const TeamWorkers&                                     workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void applyMaterialParameterMatrix(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void overwriteStateWithLinearSource(
    ComputePrecision* NOALIAS                    state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const TeamWorkers&                           workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addFluxesToTimeDerivative(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS flux,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourceDerivativeToState(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS derivativeCoefficients,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state,
    ComputePrecision                timeAverageFactor,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToTimeAveragedFlux(
    ComputePrecision* NOALIAS       timeAveragedFlux,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights,
    const TeamWorkers&              workers = TeamWorkers{}
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateToFaces(
    ComputePrecision* const NOALIAS faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS timeAveragedState,
    const Tables&                   tables,
    const TeamWorkers&              workers = TeamWorkers{}
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/LinearPredictor.cpph"
