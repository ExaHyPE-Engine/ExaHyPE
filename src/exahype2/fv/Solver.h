// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/Limits.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/RefinementVotes.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/terms/BoundaryConditions.h"
#include "exahype2/terms/InitialCondition.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace exahype2::fv {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;

  /**
   * A generated finite-volume solver carries the PDE terms and the patch layout its solver family reads.
   */
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
 * One finite-volume solver on the partition. Derived is the generated solver class: It holds
 * PatchSize, NumberOfUnknowns and NumberOfAuxiliaryVariables, the Reconstruction and
 * TimeIntegration scheme, the TimeSteppingRule and the PDE terms as static methods, and the
 * solver and its kernels read them from there. StoragePrecision is the type of the patches and
 * the faces, ComputePrecision the type the kernels compute in.
 *
 * The solver owns the solution and the previous solution, two patch pools that swap after every
 * time step, one face pool with the halo layers of both patches at every face, and the
 * exchanges that move halo layers and patches between ranks.
 *
 * Between threads the patches meet only in the face pool: projectOntoFaces() writes the outer
 * layers of every patch into its own half of each face, and updatePatches() reads the
 * neighbour's half. Between ranks the own halves of every exchanged face travel in
 * _haloExchange, and between the levels of a level jump the halves travel through the
 * interpolation and the restriction of the level jumps.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::Solver {
public:
  using FiniteVolume = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using Patches      = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using Faces        = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int PointsPerCell = FiniteVolume::VolumesPerPatch;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  Solver(double timeStepFactor, std::string name, std::vector<exahype2::plotter::DataField> plottedFields);

  void requestRefinementForGridConstruction(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void allocate(const peano4::Partition& partition);
  void initialise(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void performTimeStep(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid, exahype2::RefinementRequests& requests);
  void plot(const peano4::Partition& partition, tarch::plotter::Format format, const std::string& outputDirectory);
  void release();

  void setLimits(const exahype2::Limits& limits);
  void openErrorMeasurement(const std::string& fileName, double timeDeltaBetweenSnapshots);
  [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const;
  [[nodiscard]] const Patches& getSolution() const;
  [[nodiscard]] const Patches& getPreviousSolution() const;
  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] int getNumberOfDoFs() const;

private:
  void applyBoundaryConditions(const peano4::Partition& partition);
  void projectOntoFaces(const peano4::Partition& partition);
  [[nodiscard]] double updatePatches(
    const peano4::Partition& partition,
    peano4::Dependence       dependence,
    double                   timeStepSize
  );
  [[nodiscard]] double measureMaxEigenvalue(const peano4::Partition& partition);
  void measureError(const peano4::Partition& partition);
  void reduceMeshWidths(const peano4::Partition& partition);
  void proposeTimeStepSize(double maxEigenvalue);

  Patches _solution{FiniteVolume::ValuesPerPatch};
  Patches _previousSolution{FiniteVolume::ValuesPerPatch};
  Faces   _halo{FiniteVolume::ValuesPerFace};

  peano4::HaloExchange<StoragePrecision>      _haloExchange{_halo};
  peano4::MigrationExchange<StoragePrecision> _migration{_solution.getLocation()};

  exahype2::TimeStepping    _timeStepping;
  exahype2::RefinementVotes _votes{};
  bool                      _meshChangedSinceLastTimeStep{false};

  exahype2::ErrorMeasurement      _errorMeasurement{};
  exahype2::plotter::PatchPlotter _plotter;
  std::vector<StoragePrecision>   _solutionOnHost{};
};

#include "exahype2/fv/Solver.cpph"
