// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementRequests.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/Solver.h"
#include "exahype2/dg/rk/Stages.h"
#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/terms/BoundaryConditions.h"
#include "exahype2/terms/InitialCondition.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace exahype2::dg::rk {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;

  /**
   * A Runge-Kutta DG solver declares the terms every stage calls and the constants the node
   * loops, the stage loop and the boundary update read. Its boundary conditions run in the
   * compute precision.
   */
  template <typename SolverType, typename Storage, typename Compute>
  concept SolverConcept = exahype2::terms::HasInitialCondition<SolverType, Storage>
    and exahype2::terms::HasBoundaryConditionsWhereNeeded<SolverType, Compute>
    and exahype2::terms::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::DGOrder } -> std::convertible_to<int>;
          { SolverType::RungeKuttaOrder } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::dg::rk

/**
 * Advances the cells of one Runge-Kutta DG solver on the partition. Every stage writes the
 * volume terms of its stage state into the estimate pool, projects the stage state onto the
 * faces, solves the Riemann problems and adds the face integrals to the estimate; the final
 * state is the weighted sum of the estimates. Derived is the generated solver class as for
 * exahype2::dg::Solver, where DGOrder is the polynomial order, and adds RungeKuttaOrder.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::dg::rk::Solver : public exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base = exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision>;

  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
  template <typename Precision, typename Generated = Derived>
  using Quadrature = typename Generated::template Quadrature<Precision>;
  template <typename Precision, typename Generated = Derived>
  using DGMatrices = typename Generated::template DGMatrices<Precision>;

  using Base::UsesAdaptiveTimeStepping;

  using EstimatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );

  void allocate(const peano4::Partition& partition);
  void performTimeStep(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid, exahype2::RefinementRequests& requests);
  void release();

  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] double getTimeStepSizeFromCFL() const;
  [[nodiscard]] double getFixedTimeStepSize() const;
  [[nodiscard]] const EstimatePool& getEstimates() const;

private:
  // Stage reads the PDE terms of Derived, so only function bodies and member initialisers name
  // its constants: Derived is complete only once they are instantiated.
  using Stage = exahype2::dg::rk::Stage<Derived, ComputePrecision>;

  static constexpr int NumberOfRungeKuttaStages = Stage::Tableau::Stages;

  void openStage(const peano4::Partition& partition, int stage, double timeStepSize);
  void addFaceIntegrals(
    const peano4::Partition& partition,
    peano4::Dependence       dependence,
    int                      stage,
    double                   timeStepSize
  );
  [[nodiscard]] exahype2::dg::MaxEigenvalueStatistics combineFinalState(
    const peano4::Partition& partition,
    double                   timeStepSize
  );
  void reserveStageScratch(const peano4::Partition& partition);

  EstimatePool _estimates{Stage::EstimateValuesPerCell};

  /** One stage state per scratch chunk of the stage dispatch. */
  tarch::accelerator::Buffer<ComputePrecision> _stageScratch{tarch::MemoryLocation::Device};
};

#include "exahype2/dg/rk/Solver.cpph"
