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
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace exahype2::dg::rk {
  template <
    typename Derived,
    int DgOrder,
    int RkOrder,
    int RkStages,
    int Unknowns,
    int AuxiliaryVariables,
    typename TimeStepping>
  class Solver;
} // namespace exahype2::dg::rk

/**
 * Schedules one grid sweep per Runge-Kutta stage and, on an adaptive mesh or after a mesh
 * refinement, a mesh-refinement sweep before every stage. Derived is the generated solver class,
 * so every solver class holds its own statistics record.
 */
template <
  typename Derived,
  int DgOrder,
  int RkOrder,
  int RkStages,
  int Unknowns,
  int AuxiliaryVariables,
  typename TimeStepping>
class exahype2::dg::rk::Solver : public TimeStepping {
public:
  enum class SolverState {
    GridConstruction,
    GridInitialisation,
    Suspended,
    Plotting,
    PlottingAfterGridInitialisation,
    TimeStepAfterGridInitialisation,
    DynamicMeshRefinement,
    RungeKuttaStep
  };

  static constexpr int DGOrder                    = DgOrder;
  static constexpr int RKOrder                    = RkOrder;
  static constexpr int NumberOfRungeKuttaStages   = RkStages;
  static constexpr int NumberOfUnknowns           = Unknowns;
  static constexpr int NumberOfAuxiliaryVariables = AuxiliaryVariables;

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

  INLINE_METHOD void update(double timeStepSize, double timeStamp, double cellSize);
  INLINE_METHOD void reduceLocalUpdates();

  [[nodiscard]] INLINE_METHOD SolverState getSolverState() const;
  [[nodiscard]] INLINE_METHOD bool        isFirstGridSweepOfTimeStep() const;
  [[nodiscard]] INLINE_METHOD bool        isLastGridSweepOfTimeStep() const;
  [[nodiscard]] INLINE_METHOD bool        isGridInitialisationSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isPlottingSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isRungeKuttaSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isTimeStepAfterGridInitialisationSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isDynamicMeshRefinementSweep() const;
  [[nodiscard]] INLINE_METHOD bool        isRungeKuttaStage(int stage) const;
  [[nodiscard]] INLINE_METHOD int         getRkStage() const;
  [[nodiscard]] INLINE_METHOD int         getRkStageAfterDynamicMeshRefinement() const;
  [[nodiscard]] INLINE_METHOD double      getPreviousStageTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD bool        usesDynamicMeshRefinementSweeps() const;

  [[nodiscard]] INLINE_METHOD bool storesCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool loadsCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool storesFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool loadsFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] INLINE_METHOD bool providesCellDataToKernels(const peano4::dataview::CellMarker& marker) const;

protected:
  static thread_local exahype2::Solver::LocalUpdates _localUpdates;

  SolverState  _solverState{SolverState::GridConstruction};
  int          _rkStage{0};
  int          _rkStageAfterDynamicMeshRefinement{0};
  bool         _usesDynamicMeshRefinementSweeps{false};
  double       _previousStageTimeStepSize{0.0};
  const double _timeStepFactor;

private:
  Solver(double timeStepFactor, std::true_type adaptive);
  Solver(double timeStepFactor, std::false_type fixed);

  void decideOnDynamicMeshRefinementSweeps();
  void scheduleStage(int stage);
  void closeTimeStep();
};

#include "exahype2/dg/rk/Solver.cpph"
