// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/Limits.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/RefinementVotes.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/terms/BoundaryConditions.h"
#include "exahype2/terms/InitialCondition.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "peano4/CellConnectivity.h"
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::fv {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;

  template <typename SolverType, typename Storage>
  concept SolverConcept = exahype2::terms::HasInitialCondition<SolverType, Storage>
    and exahype2::terms::HasBoundaryConditionsWhereNeeded<SolverType, Storage>
    and exahype2::terms::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::PatchSize } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::fv

/**
 * Finite-volume solver on the cells of one rank, with the patch layout, the schemes and the PDE terms as
 * static members of Derived. StoragePrecision is the type of the patches and faces and
 * ComputePrecision the type the kernels compute in. Patches exchange data only through the face
 * pool and every patch writes its own half of each face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::Solver {
public:
  using FiniteVolumeScheme = exahype2::fv::FiniteVolumeScheme<Derived, ComputePrecision>;
  using Patches            = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using Faces              = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int PointsPerCell = FiniteVolumeScheme::VolumesPerPatch;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD constexpr auto getNodesInUnitInterval();

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  INLINE_METHOD Solver(
    double                                    timeStepFactor,
    std::string                               name,
    std::vector<exahype2::plotter::DataField> plottedFields
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void requestRefinementForGridConstruction(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity, const RuntimeParameters& parameters);

  template <typename RuntimeParameters>
  INLINE_METHOD void initialise(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void performTimeStep(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    exahype2::RefinementRequests&   requests,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void recordLastTimeStamp(
    const peano4::CellConnectivity& cellConnectivity,
    const RuntimeParameters&        parameters
  );

  INLINE_METHOD void release();
  INLINE_METHOD void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);

  INLINE_METHOD void setLimits(const exahype2::Limits& limits);
  INLINE_METHOD void openErrorMeasurement(const std::string& fileName, double timeDeltaBetweenSnapshots);
  [[nodiscard]] INLINE_METHOD const exahype2::TimeStepping& getTimeStepping() const;
  [[nodiscard]] INLINE_METHOD const Patches& getSolution() const;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfDoFs() const;

private:
  template <typename RuntimeParameters>
  INLINE_METHOD void applyBoundaryConditions(
    const peano4::CellConnectivity& cellConnectivity,
    const RuntimeParameters&        parameters
  );

  INLINE_METHOD void projectOntoFaces(const peano4::CellConnectivity& cellConnectivity);

  template <typename RuntimeParameters>
  [[nodiscard]] INLINE_METHOD double updatePatches(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStepSize,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void correctFluxesAtLevelJumps(
    const peano4::CellConnectivity& cellConnectivity,
    double                          timeStepSize,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  [[nodiscard]] INLINE_METHOD double measureMaxEigenvalue(
    const peano4::CellConnectivity& cellConnectivity,
    const RuntimeParameters&        parameters
  );

  template <typename RuntimeParameters>
  INLINE_METHOD void measureError(
    const peano4::CellConnectivity& cellConnectivity,
    const RuntimeParameters&        parameters
  );

  INLINE_METHOD void measureMeshWidths(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void proposeTimeStepSize(double maxEigenvalue);
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeFromCFL() const;
  [[nodiscard]] INLINE_METHOD double getFixedTimeStepSize() const;

  Patches _solution{FiniteVolumeScheme::ValuesPerPatch};
  Faces   _halo{FiniteVolumeScheme::ValuesPerFace};

  peano4::HaloExchange<StoragePrecision>      _haloExchange{_halo};
  peano4::HaloExchange<StoragePrecision>      _fluxCorrectionExchange{_halo, peano4::FaceKind::Coarse};
  peano4::MigrationExchange<StoragePrecision> _migration{_solution.getLocation()};

  exahype2::TimeStepping    _timeStepping;
  exahype2::RefinementVotes _votes{};
  bool                      _meshChangedSinceLastTimeStep{false};

  exahype2::ErrorMeasurement      _errorMeasurement{};
  exahype2::plotter::PatchPlotter _plotter;
  Patches                         _solutionOnHost{FiniteVolumeScheme::ValuesPerPatch, tarch::MemoryLocation::Heap};
};

#include "exahype2/fv/Solver.cpph"
