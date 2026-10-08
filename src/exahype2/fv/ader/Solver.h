// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/Solver.h"
#include "exahype2/fv/ader/PatchPredictor.h"
#include "exahype2/fv/ader/Scheme.h"
#include "peano4/HaloExchange.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::fv::ader {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;
} // namespace exahype2::fv::ader

/**
 * The ADER-WENO solver family after Dumbser et al. Every time step predicts every volume of every
 * patch from its WENO polynomial, adds the fluxes inside the patch at once and writes the predicted
 * states and fluxes of the faces of the patch into the face pools. The ranks exchange these halves,
 * the Riemann problem of every face runs once, and every patch adds the numerical fluxes on its
 * faces. Level jumps interpolate the predicted halves of the coarse faces and restrict the fluxes of
 * the fine faces, as ADER-DG does per volume face. A diverged prediction terminates the run, and the
 * global master warns about the volumes whose Picard iteration stopped at its sweep limit.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::ader::Solver : public exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base      = exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision>;
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  INLINE_METHOD Solver(
    double                                    timeStepFactor,
    std::string                               name,
    std::vector<exahype2::plotter::DataField> plottedFields
  );

  INLINE_METHOD void allocate(const peano4::Partition& partition);
  INLINE_METHOD void performTimeStep(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  INLINE_METHOD void regrid(
    const peano4::Partition&      partition,
    const peano4::Regrid&         regrid,
    exahype2::RefinementRequests& requests
  );
  INLINE_METHOD void release();
  INLINE_METHOD void contributeTo(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void adoptFrom(const tarch::mpi::ReductionRecord& record);
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

private:
  INLINE_METHOD void reserveFaces(const peano4::Partition& partition);
  [[nodiscard]] INLINE_METHOD double predictPatches(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD double solveRiemannProblems(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD double correctPatches(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );

  StatePool _faceStates{Scheme::StateValuesPerFace};
  FluxPool  _faceFluxes{Scheme::FluxValuesPerFace};

  peano4::HaloExchange<ComputePrecision> _stateExchange{_faceStates};
  peano4::HaloExchange<ComputePrecision> _fluxExchange{_faceFluxes};
  peano4::HaloExchange<ComputePrecision> _restrictedFluxExchange{_faceFluxes, peano4::FaceKind::Coarse};

  exahype2::fv::ader::PatchPredictor<StoragePrecision, ComputePrecision> _predictor{};

  /**
   * The number of volumes of this rank whose Picard iteration stopped at its sweep limit since the
   * last reduction.
   */
  int _unconvergedPredictions{0};
  int _unconvergedPredictionsSlot{0};
};

#include "exahype2/fv/ader/Solver.cpph"
