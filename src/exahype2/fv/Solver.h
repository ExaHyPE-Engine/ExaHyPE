// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/AdaptiveTimeStepping.h"
#include "exahype2/FixedTimeStepping.h"
#include "exahype2/Solver.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/NonCriticalAssertions.h"

namespace exahype2::fv {
  template <typename Derived, int PatchSize, int Unknowns, int AuxiliaryVariables, typename TimeStepping>
  class Solver;
} // namespace exahype2::fv

/**
 * Holds the sweep state of a finite-volume solver and derives its time step size. Derived is the
 * generated solver class, so every solver class holds its own statistics record.
 */
template <typename Derived, int PatchSize, int Unknowns, int AuxiliaryVariables, typename TimeStepping>
class exahype2::fv::Solver : public TimeStepping {
public:
  enum class SolverState { GridConstruction, GridInitialisation, TimeStep, Plotting, Suspended };

  static constexpr int NumberOfFiniteVolumesPerAxisPerPatch = PatchSize;
  static constexpr int NumberOfUnknowns                     = Unknowns;
  static constexpr int NumberOfAuxiliaryVariables           = AuxiliaryVariables;

  static constexpr bool UsesAdaptiveTimeStepping = std::is_same_v<TimeStepping, exahype2::AdaptiveTimeStepping>;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver() override               = default;

  explicit Solver(double timeStepFactor);

  static std::string toString(SolverState state);

  void startGridConstructionStep() override;
  void finishGridConstructionStep() override;
  void startGridInitialisationStep() override;
  void finishGridInitialisationStep() override;
  void suspendSolversForOneGridSweep() override;
  void startTimeStep() override;
  void finishTimeStep() override;
  void startPlottingStep() override;
  void finishPlottingStep() override;

  [[nodiscard]] double getMaxPatchSize() const override;
  [[nodiscard]] double getMinPatchSize() const override;
  [[nodiscard]] int    getNumberOfDoFs() const override;
  [[nodiscard]] bool   mayPlot() const override;

  INLINE_METHOD void update(double timeStepSize, double timeStamp, double minPatchSize, double maxPatchSize);
  INLINE_METHOD void updateTimeStampAndPatchSize(double timeStamp, double minPatchSize, double maxPatchSize);
  INLINE_METHOD void reduceLocalUpdates();

  [[nodiscard]] INLINE_METHOD SolverState getSolverState() const;
  [[nodiscard]] INLINE_METHOD bool        isFirstGridSweepOfTimeStep() const;
  [[nodiscard]] INLINE_METHOD bool        isLastGridSweepOfTimeStep() const;
  [[nodiscard]] INLINE_METHOD bool        isGridInitialisationSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isTimeStepSweep() const;

  [[nodiscard]] INLINE_METHOD bool storesCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool loadsCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool storesFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool loadsFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool providesCellDataToKernels(const peano4::dataview::CellMarker& marker) const;

protected:
  /**
   * Returns the largest mesh width the runtime configuration admits for one finite volume. A
   * fixed time step scales with the ratio of the smallest mesh width to it.
   */
  [[nodiscard]] virtual double maxAdmissibleMeshWidth() const = 0;

  static thread_local exahype2::Solver::LocalUpdates _localUpdates;

  SolverState  _solverState{SolverState::GridConstruction};
  const double _timeStepFactor;

private:
  Solver(double timeStepFactor, std::true_type adaptive);
  Solver(double timeStepFactor, std::false_type fixed);

  void closeTimeStep();
};

#include "exahype2/fv/Solver.cpph"
