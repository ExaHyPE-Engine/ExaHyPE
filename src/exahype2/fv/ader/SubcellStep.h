// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/limiting/SubcellScheme.h"
#include "exahype2/fv/ader/PatchPredictor.h"
#include "exahype2/fv/ader/Scheme.h"
#include "peano4/HaloExchange.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class SubcellStep;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct SolveSubcellRiemannProblems;
} // namespace exahype2::fv::ader

/**
 * The step of the subcell limiter of the ADER-DG solver Derived with ADER-WENO, patch by patch as
 * exahype2::fv::ader::Solver advances its patches. Every recomputing cell predicts its subgrid, the
 * ranks exchange the predicted face halves, the Riemann problems run on the faces with a
 * recomputing side, and every recomputing cell corrects its subgrid. On a face with one recomputing
 * side, that side applies the numerical flux of the Galerkin step, averaged over each volume face,
 * so the subgrid and the Galerkin cell beside it exchange the same flux.
 *
 * The predicted face halves cover every face of the partition.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::ader::SubcellStep final {
public:
  using Kernel    = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using Region    = exahype2::dg::limiting::SubcellRegion<StoragePrecision, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
  template <typename Generated = Derived>
  using Subgrid = typename exahype2::dg::limiting::SubcellScheme<Generated, ComputePrecision>::template SubgridSolver<
    typename Generated::Reconstruction>;
  template <typename Generated = Derived>
  using Scheme = exahype2::fv::ader::Scheme<Subgrid<Generated>, ComputePrecision>;

  INLINE_METHOD void rebuild(const peano4::Partition& partition);
  INLINE_METHOD void release();
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

  template <typename ExchangeHalo>
  INLINE_METHOD void advance(
    const peano4::Partition& partition,
    const Region&            region,
    const ExchangeHalo&      exchangeHalo,
    double                   timeStamp,
    double                   timeStepSize
  );

private:
  INLINE_METHOD void predict(
    const peano4::Partition& partition,
    const Region&            region,
    tarch::Span<const int>   cells,
    double                   timeStamp,
    double                   timeStepSize
  );
  INLINE_METHOD void solveRiemannProblems(
    const peano4::Partition& partition,
    const Region&            region,
    peano4::Adjacency        adjacency,
    double                   timeStamp,
    double                   timeStepSize
  );
  INLINE_METHOD void correct(
    const peano4::Partition& partition,
    const Region&            region,
    tarch::Span<const int>   cells,
    double                   timeStamp,
    double                   timeStepSize
  );

  StatePool _faceStates{Scheme<>::StateValuesPerFace};
  StatePool _faceFluxes{Scheme<>::FluxValuesPerFace};

  peano4::HaloExchange<ComputePrecision> _stateExchange{_faceStates};
  peano4::HaloExchange<ComputePrecision> _fluxExchange{_faceFluxes};
  peano4::HaloExchange<ComputePrecision> _restrictedFluxExchange{_faceFluxes, peano4::FaceKind::Coarse};

  exahype2::fv::ader::PatchPredictor<StoragePrecision, ComputePrecision> _predictor{};
};

TARGET_DECLARE
/**
 * Solves the Riemann problems on every listed face with two recomputing sides or on the domain
 * boundary and reduces the largest wave speed. On a face with one recomputing side, that side's flux
 * half takes the flux that it applied in the Galerkin step from appliedFluxes instead.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::SolveSubcellRiemannProblems final {
  using Step       = exahype2::fv::ader::SubcellStep<Derived, StoragePrecision, ComputePrecision>;
  using Kernel     = typename Step::Kernel;
  using Scheme     = typename Step::template Scheme<>;
  using StatePool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  static constexpr int AppliedFluxValuesPerHalf = Kernel::AppliedFluxValuesPerFace / 2;

  peano4::Partition              partition{};
  tarch::Span<const int>         faces{};
  typename StatePool::View       faceStates{};
  typename StatePool::View       faceFluxes{};
  typename StatusPool::ConstView faceStatuses{};
  typename StatePool::ConstView  appliedFluxes{};
  const double*                  nodesToSubcells{nullptr};
  double                         timeStamp{0.0};
  double                         timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, double& accumulator) const;

private:
  INLINE_METHOD GPU_CALLABLE_METHOD void imposeAppliedFlux(
    const ComputePrecision* NOALIAS appliedHalf,
    ComputePrecision* NOALIAS       fluxHalf,
    int                             normal
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/SubcellStep.cpph"
