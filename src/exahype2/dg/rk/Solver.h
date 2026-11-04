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
#include "peano4/CellConnectivity.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace exahype2::dg::rk {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;

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
 * The Runge-Kutta DG solver family. Every stage writes one estimate per cell and the new solution
 * adds the weighted estimates. Derived also declares RungeKuttaOrder.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::dg::rk::Solver : public exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision> {
public:
  using Base = exahype2::dg::Solver<Derived, StoragePrecision, ComputePrecision>;

  template <typename Precision, typename Generated = Derived>
  using Quadrature = typename Generated::template Quadrature<Precision>;
  template <typename Precision, typename Generated = Derived>
  using DGMatrices = typename Generated::template DGMatrices<Precision>;

  using Base::UsesAdaptiveTimeStepping;

  using EstimatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  Solver()                               = delete;
  Solver(const Solver& other)            = delete;
  Solver(Solver&& other)                 = delete;
  Solver& operator=(const Solver& other) = delete;
  Solver& operator=(Solver&& other)      = delete;
  ~Solver()                              = default;

  INLINE_METHOD Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );

  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void performTimeStep(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void release();

  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeFromCFL() const;
  [[nodiscard]] INLINE_METHOD double getFixedTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD const EstimatePool& getEstimates() const;

private:
  using Stage = exahype2::dg::rk::Stage<Derived, ComputePrecision>;

  static constexpr int NumberOfRungeKuttaStages = Stage::Tableau::Stages;

  INLINE_METHOD void openStage(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    int                             stage,
    double                          timeStepSize
  );
  [[nodiscard]] INLINE_METHOD exahype2::dg::MaxEigenvalueStatistics combineFinalState(
    const peano4::CellConnectivity& cellConnectivity,
    double                          timeStepSize
  );
  INLINE_METHOD void reserveStageScratch(const peano4::CellConnectivity& cellConnectivity);

  EstimatePool _estimates{Stage::EstimateValuesPerCell};

  tarch::accelerator::Buffer<ComputePrecision> _stageScratch{tarch::MemoryLocation::Device};
};

#include "exahype2/dg/rk/Solver.cpph"
