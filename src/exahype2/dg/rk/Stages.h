// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/terms/DiffusiveFlux.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::rk {
  template <int Order>
  struct ButcherTableau;

  template <typename Derived, typename ComputePrecision>
  class Stage;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct OpenStage;

  template <typename Derived, typename ComputePrecision>
  struct AddStageFaceIntegrals;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct CombineStages;
} // namespace exahype2::dg::rk

namespace exahype2::dg::rk::tests {
  class StagesTest;
} // namespace exahype2::dg::rk::tests

TARGET_DECLARE
/**
 * The explicit Runge-Kutta scheme of one order as a Butcher tableau: Nodes holds the fraction
 * of the time step every stage advances to, Matrix the weight of every earlier stage in a
 * stage, and Weights the weight of every stage in the final state. Order one is the explicit
 * Euler scheme, order two the midpoint scheme, order three Kutta's scheme and order four the
 * three-eighths scheme.
 */
template <>
struct exahype2::dg::rk::ButcherTableau<1> final {
  static constexpr int                                            Stages = 1;
  static constexpr std::array<double, Stages>                     Nodes{0.0};
  static constexpr std::array<double, Stages>                     Weights{1.0};
  static constexpr std::array<std::array<double, Stages>, Stages> Matrix{{{0.0}}};
};

template <>
struct exahype2::dg::rk::ButcherTableau<2> final {
  static constexpr int                                            Stages = 2;
  static constexpr std::array<double, Stages>                     Nodes{0.0, 0.5};
  static constexpr std::array<double, Stages>                     Weights{0.0, 1.0};
  static constexpr std::array<std::array<double, Stages>, Stages> Matrix{{{0.0, 0.0}, {0.5, 0.0}}};
};

template <>
struct exahype2::dg::rk::ButcherTableau<3> final {
  static constexpr int                                            Stages = 3;
  static constexpr std::array<double, Stages>                     Nodes{0.0, 0.5, 1.0};
  static constexpr std::array<double, Stages>                     Weights{1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0};
  static constexpr std::array<std::array<double, Stages>, Stages> Matrix{
    {{0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {-1.0, 2.0, 0.0}}
  };
};

template <>
struct exahype2::dg::rk::ButcherTableau<4> final {
  static constexpr int                                            Stages = 4;
  static constexpr std::array<double, Stages>                     Nodes{0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0};
  static constexpr std::array<double, Stages>                     Weights{1.0 / 8.0, 3.0 / 8.0, 3.0 / 8.0, 1.0 / 8.0};
  static constexpr std::array<std::array<double, Stages>, Stages> Matrix{
    {{0.0, 0.0, 0.0, 0.0}, {1.0 / 3.0, 0.0, 0.0, 0.0}, {-1.0 / 3.0, 1.0, 0.0, 0.0}, {1.0, -1.0, 1.0, 0.0}}
  };
};

/**
 * One Runge-Kutta stage of one cell of Derived, the generated solver class, whose RungeKuttaOrder
 * picks the Butcher tableau. The estimates of a cell hold NumberOfUnknowns values per node, one
 * block per stage.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::rk::Stage final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tableau    = exahype2::dg::rk::ButcherTableau<Derived::RungeKuttaOrder>;
  using Weights    = std::array<double, Tableau::Stages>;

  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateFlux   = exahype2::terms::HasFlux<Derived, ComputePrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateSource = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;

  static constexpr int UnknownsPerStage      = CellLayout::NodesPerCell * CellLayout::NumberOfUnknowns;
  static constexpr int EstimateValuesPerCell = Tableau::Stages * UnknownsPerStage;
  static constexpr int StateValuesPerCell    = CellLayout::NodesPerCell * CellLayout::NumberOfData;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void open(
    const Weights&                               weights,
    const StoragePrecision* NOALIAS              cellState,
    ComputePrecision* NOALIAS                    estimates,
    ComputePrecision* NOALIAS                    stageState,
    int                                          stage,
    ComputePrecision* const NOALIAS              faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       stageTimeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void addFaceIntegrals(
    ComputePrecision* NOALIAS                    estimates,
    int                                          stage,
    const ComputePrecision* const                faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const ComputePrecision* NOALIAS              inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStepSize
  );

  template <bool ReportEigenvalues, typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics close(
    const Weights&                               weights,
    StoragePrecision*                            cellState,
    const ComputePrecision* NOALIAS              estimates,
    const StoragePrecision* NOALIAS              stateQuadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

private:
  friend class exahype2::dg::rk::tests::StagesTest;

  template <typename StoragePrecision, typename TargetPrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void combineEstimates(
    const Weights&                  weights,
    const StoragePrecision*         cellState,
    const ComputePrecision* NOALIAS estimates,
    TargetPrecision*                target
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void volumeIntegral(
    const ComputePrecision* NOALIAS              stageState,
    ComputePrecision* NOALIAS                    estimate,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void projectOntoFaces(
    const ComputePrecision* NOALIAS              stageState,
    ComputePrecision* const NOALIAS              faceStates[peano4::FacesPerCell],
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
};

/**
 * Opens the stage on every listed cell: The estimate of the stage takes the volume terms of the
 * stage state, and the cell's own half of every face takes the face states and face fluxes of
 * that state. stageScratch holds one stage state per scratch chunk of the dispatch.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::rk::OpenStage final {
  using Kernel     = exahype2::dg::rk::Stage<Derived, ComputePrecision>;
  using CellLayout = typename Kernel::CellLayout;

  using CellPool     = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using EstimatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatePool    = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool     = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView solution{};
  typename EstimatePool::View  estimates{};
  typename StatePool::View     faceStates{};
  typename FluxPool::View      faceFluxes{};
  typename Kernel::Weights     weights{};
  const ComputePrecision*      quadratureNodes{nullptr};
  const ComputePrecision*      derivativeOperator{nullptr};
  const ComputePrecision*      leftFaceCoefficients{nullptr};
  const ComputePrecision*      rightFaceCoefficients{nullptr};
  ComputePrecision*            stageScratch{nullptr};
  int                          stage{0};
  double                       stageTimeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Adds the face integrals of every listed cell to the estimate of the stage.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::rk::AddStageFaceIntegrals final {
  using Kernel     = exahype2::dg::rk::Stage<Derived, ComputePrecision>;
  using CellLayout = typename Kernel::CellLayout;

  using EstimatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool     = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename EstimatePool::View  estimates{};
  typename FluxPool::ConstView faceFluxes{};
  const ComputePrecision*      faceQuadratureWeights{nullptr};
  const ComputePrecision*      inverseCellQuadratureWeights{nullptr};
  const ComputePrecision*      leftFaceCoefficients{nullptr};
  const ComputePrecision*      rightFaceCoefficients{nullptr};
  int                          stage{0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Writes the weighted sum of the estimates over the solution of every listed cell, runs the
 * postprocessing of the patch and reduces the wave-speed maxima of the new solution of an
 * adaptive rule.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::rk::CombineStages final {
  using Kernel = exahype2::dg::rk::Stage<Derived, ComputePrecision>;

  static constexpr bool UsesAdaptiveTimeStepping = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  using CellPool     = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using EstimatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition                partition{};
  tarch::Span<const int>           cells{};
  typename CellPool::View          solution{};
  typename EstimatePool::ConstView estimates{};
  typename Kernel::Weights         weights{};
  const StoragePrecision*          stateQuadraturePoints{nullptr};
  double                           timeStamp{0.0};
  double                           timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/rk/Stages.cpph"
