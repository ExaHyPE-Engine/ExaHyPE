// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/Limits.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/RefinementVotes.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/Patch.h"
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
 * The data and the phases that every finite-volume solver family shares, with the patch layout, the
 * reconstruction and the PDE terms as static members of Derived. StoragePrecision is the type of the
 * patches and faces, and ComputePrecision the type the kernels compute in. Patches exchange their
 * solution only through the halo pool, where every patch writes its own half of each face. A family
 * such as exahype2::fv::singlestep::Solver or exahype2::fv::ader::Solver adds performTimeStep().
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::Solver {
public:
  using Patch = exahype2::fv::Patch<Derived, ComputePrecision>;
  using Patches            = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using Faces              = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int PointsPerCell = Patch::VolumesPerPatch;

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

  INLINE_METHOD void requestRefinementForGridConstruction(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void initialise(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory
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

  static INLINE_METHOD void evaluateVolumeCentres(
    const StoragePrecision*        patch,
    const StoragePrecision* const* faces,
    double*                        values
  );

protected:
  INLINE_METHOD void applyBoundaryConditions(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void interpolateCoarseHalos(const peano4::CellConnectivity& cellConnectivity, peano4::Adjacency adjacency);
  INLINE_METHOD void closeTimeStep(
    const peano4::CellConnectivity&      cellConnectivity,
    double                        timeStepSize,
    double                        maxEigenvalue,
    exahype2::RefinementRequests& requests
  );
  template <int PointsPerAxis, typename EvaluatePatch>
  INLINE_METHOD void plotReconstruction(
    const peano4::CellConnectivity&                   cellConnectivity,
    tarch::plotter::Format                            format,
    const std::string&                                outputDirectory,
    const EvaluatePatch&                              evaluatePatch,
    const tarch::accelerator::ChunkedMemoryPool<int>* volumeStatuses = nullptr,
    const char*                                       statusName     = nullptr
  );

  Patches _solution{Patch::ValuesPerPatch};
  Faces   _halo{Patch::ValuesPerFace};

  peano4::HaloExchange<StoragePrecision> _haloExchange{_halo};

  exahype2::TimeStepping _timeStepping;

private:
  INLINE_METHOD void projectOntoFaces(const peano4::CellConnectivity& cellConnectivity);
  [[nodiscard]] INLINE_METHOD double measureMaxEigenvalue(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void measureError(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void measureMeshWidths(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void proposeTimeStepSize(double maxEigenvalue);
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeFromCFL() const;
  [[nodiscard]] INLINE_METHOD double getFixedTimeStepSize() const;
  INLINE_METHOD void fillHalo(const peano4::CellConnectivity& cellConnectivity);
  template <int ElementsPerAxis, int PointsPerAxis, typename Evaluate>
  INLINE_METHOD void plotPatch(
    const peano4::CellConnectivity&                   cellConnectivity,
    tarch::plotter::Format                            format,
    const std::string&                                outputDirectory,
    const Evaluate&                                   evaluate,
    const tarch::accelerator::ChunkedMemoryPool<int>* volumeStatuses,
    const char*                                       statusName,
    const double*                                     pointPositions = nullptr
  );

  peano4::MigrationExchange<StoragePrecision> _migration{_solution.getLocation()};

  exahype2::RefinementVotes _votes{};
  bool                      _meshChangedSinceLastTimeStep{false};

  exahype2::ErrorMeasurement      _errorMeasurement{};
  exahype2::plotter::PatchPlotter _plotter;
  Patches                         _solutionOnHost{Patch::ValuesPerPatch, tarch::MemoryLocation::Heap};
  Faces                           _haloOnHost{Patch::ValuesPerFace, tarch::MemoryLocation::Heap};
};

#include "exahype2/fv/Solver.cpph"
