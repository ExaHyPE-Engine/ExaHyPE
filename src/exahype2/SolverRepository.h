// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonNegativeReduction.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  template <typename... Solvers>
  class SolverRepository;
} // namespace exahype2

/**
 * Calls one method on every solver of a project and reduces the answers. The repository holds
 * references, so every solver outlives it.
 */
template <typename... Solvers>
class exahype2::SolverRepository final {
public:
  explicit SolverRepository(Solvers&... solvers);

  SolverRepository(const SolverRepository&)            = delete;
  SolverRepository(SolverRepository&&)                 = delete;
  SolverRepository& operator=(const SolverRepository&) = delete;
  SolverRepository& operator=(SolverRepository&&)      = delete;
  ~SolverRepository()                                  = default;

  [[nodiscard]] INLINE_METHOD double getMinTimeStamp() const;
  [[nodiscard]] INLINE_METHOD double getMaxTimeStamp() const;
  [[nodiscard]] INLINE_METHOD double getMinTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD double getMaxTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD int    getNumberOfDoFs() const;

  [[nodiscard]] INLINE_METHOD bool mayPlot() const;
  [[nodiscard]] INLINE_METHOD bool isFirstGridSweepOfTimeStep() const;
  [[nodiscard]] INLINE_METHOD bool isLastGridSweepOfTimeStep() const;

  INLINE_METHOD void startGridConstructionStep();
  INLINE_METHOD void finishGridConstructionStep();
  INLINE_METHOD void startGridInitialisationStep();
  INLINE_METHOD void finishGridInitialisationStep();
  INLINE_METHOD void startDynamicMeshRefinementStep();
  INLINE_METHOD void finishDynamicMeshRefinementStep();
  INLINE_METHOD void startTimeStep();
  INLINE_METHOD void finishTimeStep();
  INLINE_METHOD void startPlottingStep();
  INLINE_METHOD void finishPlottingStep();
  INLINE_METHOD void suspendSolversForOneGridSweep();
  INLINE_METHOD void startSimulation();
  INLINE_METHOD void finishSimulation();

private:
  template <typename Operation>
  INLINE_METHOD void forEachSolver(const Operation& operation);

  template <typename Operation>
  INLINE_METHOD void forEachSolver(const Operation& operation) const;

  std::tuple<Solvers&...> _solvers;
};

#include "exahype2/SolverRepository.cpph"
