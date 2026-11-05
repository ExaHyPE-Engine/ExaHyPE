// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/Solver.h"
#include "exahype2/fv/ader/Fallback.h"
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
 *
 * With a fallback reconstruction in Derived, every patch keeps its previous solution, judges its
 * corrected volumes with exahype2::fv::ader::Fallback and repeats a troubled step with the
 * fallback scheme (see CorrectPatches). The ranks exchange the statuses on the faces while the
 * local patches beside a repeating patch take its fallback flux (see FixUpPatches), and then the
 * remote ones do. A diverged prediction then leaves NaN in its patch, which repeats its step.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::ader::Solver : public exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base      = exahype2::fv::Solver<Derived, StoragePrecision, ComputePrecision>;
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool   = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
  template <typename Generated = Derived>
  using FallbackSolver = std::conditional_t<
    exahype2::fv::ader::fallsBack<Generated>(),
    exahype2::fv::ader::FallbackSolver<Generated, typename exahype2::fv::ader::FallbackReconstructionOf<Generated>::Type>,
    void>;

  static constexpr bool HasLimiter = exahype2::fv::ader::fallsBack<Derived>();

  INLINE_METHOD Solver(
    double                                    timeStepFactor,
    std::string                               name,
    std::vector<exahype2::plotter::DataField> plottedFields
  );

  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void performTimeStep(const peano4::CellConnectivity& cellConnectivity, exahype2::RefinementRequests& requests);
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity&      cellConnectivity,
    const peano4::Regrid&         regrid,
    exahype2::RefinementRequests& requests
  );
  INLINE_METHOD void release();
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory
  );
  INLINE_METHOD void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfTroubledCells() const
    requires(exahype2::fv::ader::fallsBack<Derived>());

private:
  INLINE_METHOD void reserveFaces(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void reserveVolumeStatuses(int numberOfSlots);
  [[nodiscard]] INLINE_METHOD double predictPatches(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD double solveRiemannProblems(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD double correctPatches(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD double fixUpPatches(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency        adjacency,
    double                   timeStepSize
  );
  [[nodiscard]] INLINE_METHOD auto getCorrection(
    const peano4::CellConnectivity& cellConnectivity,
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

  typename Base::Patches    _previousSolution{Base::Patch::ValuesPerPatch};
  StatusPool                _statuses{1};
  StatusPool                _faceStatuses{2 * exahype2::fv::ader::getFallbackStatusesPerHalf<Derived>()};
  peano4::HaloExchange<int> _statusExchange{_faceStatuses};
  StatusPool                _volumeStatuses{Scheme::VolumesPerPatch};
  StatePool                 _interiorUpdates{Scheme::InteriorUpdateValues};
  StatusPool                _volumeStatusesOnHost{Scheme::VolumesPerPatch, tarch::MemoryLocation::Heap};

  /**
   * The troubled patches or volumes of this rank that repeated their step with the fallback scheme
   * since the last reduction, the reduction slot of their sum, and those of all ranks in the time
   * step of the last read reduction.
   */
  int _localTroubledCells{0};
  int _troubledCellsSlot{0};
  int _globalTroubledCells{0};
};

#include "exahype2/fv/ader/Solver.cpph"
