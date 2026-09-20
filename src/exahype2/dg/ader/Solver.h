// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/AdaptiveTimeStepping.h"
#include "exahype2/FixedTimeStepping.h"
#include "exahype2/Solver.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/LimitingSolverState.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace exahype2::dg::ader {
  template <
    typename Derived,
    int Order,
    int Unknowns,
    int AuxiliaryVariables,
    typename TimeStepping,
    bool SubcellLimiter,
    int  PointSources>
  class Solver;

  constexpr double cflFactor(int order);
} // namespace exahype2::dg::ader

/**
 * Holds the sweep state of an ADER-DG solver, with or without the subcell limiter, and derives
 * its time step size. Derived is the generated solver class, so every solver class holds its own
 * statistics record and point-source locations.
 */
template <
  typename Derived,
  int Order,
  int Unknowns,
  int AuxiliaryVariables,
  typename TimeStepping,
  bool SubcellLimiter,
  int  PointSources>
class exahype2::dg::ader::Solver : public TimeStepping {
public:
  enum class SolverState {
    GridConstruction,
    GridInitialisation,
    InitialPrediction,
    PredictionOnHangingCells,
    TimeStep,
    Plotting,
    Suspended
  };

  static_assert(Order >= 0 and Order < 16, "The CFL factors of the ADER-DG scheme cover the orders 0 to 15.");

  static constexpr int    DGOrder                    = Order;
  static constexpr int    NumberOfUnknowns           = Unknowns;
  static constexpr int    NumberOfAuxiliaryVariables = AuxiliaryVariables;
  static constexpr int    NumberOfPointSources       = PointSources;
  static constexpr double PNPM                       = cflFactor(Order);
  /**
   * The Picard iteration of the predictor stops once the Euclidean distance between two
   * iterates falls below this value.
   */
  static constexpr double PicardResidualTolerance = 1e-7;

  static constexpr bool UsesAdaptiveTimeStepping = std::is_same_v<TimeStepping, exahype2::AdaptiveTimeStepping>;
  static constexpr bool UsesSubcellLimiter       = SubcellLimiter;

  static double pointSourceLocation[PointSources == 0 ? 1 : PointSources][DIMENSIONS];

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

  INLINE_METHOD void update(
    double timeStepSize,
    double timeStamp,
    double minCellSize,
    double maxCellSize,
    bool   hasHangingFaces
  );
  INLINE_METHOD void reduceLocalUpdates();

  [[nodiscard]] SolverState                       getSolverState() const;
  [[nodiscard]] exahype2::dg::LimitingSolverState getLimitingSolverState() const;
  [[nodiscard]] bool                              isFirstGridSweepOfTimeStep() const;
  [[nodiscard]] bool                              isLastGridSweepOfTimeStep() const;
  [[nodiscard]] bool                              isGridInitialisationSweep() const;
  [[nodiscard]] bool                              isPlottingSweep() const;
  [[nodiscard]] bool                              isFinalSubcellSpreadSweep() const;
  [[nodiscard]] long                              getNumberOfTroubledCells() const;

  [[nodiscard]] bool isPredictionOnHangingCellsSweep() const;
  [[nodiscard]] bool isLimitingSweep(exahype2::dg::LimitingSolverState state) const;

  [[nodiscard]] bool storesCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] bool loadsCellData(const peano4::dataview::CellMarker& marker) const;
  [[nodiscard]] bool storesFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] bool loadsFaceData(const peano4::dataview::FaceMarker& marker) const;
  [[nodiscard]] bool providesCellDataToKernels(const peano4::dataview::CellMarker& marker) const;

  void markCellAsTroubled();

  [[nodiscard]] bool reducesStatisticsThisSweep() const;

protected:
  [[nodiscard]] virtual double maxAdmissibleMeshWidth() const = 0;

  static thread_local exahype2::Solver::LocalUpdates _localUpdates;

  SolverState                 _solverState{SolverState::GridConstruction};
  exahype2::dg::LimitingSweep _limitingSweep{};
  std::atomic<long>           _numberOfTroubledCellsThisTimeStep{0};
  const double                _timeStepFactor;

private:
  Solver(double timeStepFactor, std::true_type adaptive);
  Solver(double timeStepFactor, std::false_type fixed);

  [[nodiscard]] exahype2::dg::LimitingBranchDecision branchDecision() const;
  [[nodiscard]] SolverState                          successorAfterInterruption() const;
  [[nodiscard]] SolverState                          successorOfTimeStep() const;
  [[nodiscard]] bool                                 isCorrectorSweep() const;
  void                                               advanceLimitingState(bool& resumesFromSuspension);
  void                                               closeTimeStep();
};

#include "exahype2/dg/ader/Solver.cpph"
