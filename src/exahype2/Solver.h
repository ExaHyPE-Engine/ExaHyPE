// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonNegativeStatistics.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/ScalarOperations.h"

namespace exahype2 {
  class Solver;
} // namespace exahype2

/**
 * Holds the time stamps, time step sizes and mesh widths of a solver, merges the extrema of
 * the traversal tasks and reduces them over the ranks.
 */
class exahype2::Solver {
public:
  /**
   * Holds the extrema one traversal task collects until mergeLocalUpdates() moves them into the
   * solver. The time step statistics keep an invalid observation once one arrived.
   */
  struct LocalUpdates final {
    double                minTimeStamp{std::numeric_limits<double>::infinity()};
    double                maxTimeStamp{0.0};
    double                minMeshWidth{std::numeric_limits<double>::infinity()};
    double                maxMeshWidth{0.0};
    NonNegativeStatistics timeStepSizeStatistics{};
    bool                  hasHangingFaces{false};
    bool                  empty{true};

    INLINE_METHOD void addTimeStep(double timeStepSize);
    INLINE_METHOD void addTimeStamp(double timeStamp);
    INLINE_METHOD void addMeshWidths(double minimum, double maximum);
  };

  Solver()                         = default;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  virtual ~Solver()                = default;

  [[nodiscard]] virtual double getMinTimeStamp() const;
  [[nodiscard]] virtual double getMaxTimeStamp() const;
  [[nodiscard]] virtual double getMinTimeStepSize() const;
  [[nodiscard]] virtual double getMaxTimeStepSize() const;
  [[nodiscard]] virtual double getMinCellSize() const;
  [[nodiscard]] virtual double getMaxCellSize() const;

  [[nodiscard]] INLINE_METHOD bool hasStoredFaceData() const;

  void mergeLocalUpdates(LocalUpdates& localUpdates);

  virtual void startGridConstructionStep()  = 0;
  virtual void finishGridConstructionStep() = 0;

  virtual void suspendSolversForOneGridSweep() = 0;

  virtual void startGridInitialisationStep()  = 0;
  virtual void finishGridInitialisationStep() = 0;

  virtual void startTimeStep()  = 0;
  virtual void finishTimeStep() = 0;

  virtual void startDynamicMeshRefinementStep();
  virtual void finishDynamicMeshRefinementStep();

  virtual void startPlottingStep()  = 0;
  virtual void finishPlottingStep() = 0;

  [[nodiscard]] virtual double getMaxPatchSize() const = 0;
  [[nodiscard]] virtual double getMinPatchSize() const = 0;

  [[nodiscard]] virtual int getNumberOfDoFs() const = 0;

  /**
   * Returns whether a plot between the sweeps of this solver reads a consistent solution.
   */
  [[nodiscard]] virtual bool mayPlot() const = 0;

  virtual void startSimulation()  = 0;
  virtual void finishSimulation() = 0;

protected:
  void resetTimeStepStatistics();

  void reduceTimeStepStatisticsAcrossRanks();

  void commitTimeStepStatistics();

  double _minTimeStamp{0.0};
  double _maxTimeStamp{0.0};

  double _minTimeStampThisTimeStep{std::numeric_limits<double>::infinity()};
  double _maxTimeStampThisTimeStep{0.0};

  double _minMeshWidth{std::numeric_limits<double>::infinity()};
  double _maxMeshWidth{0.0};

  double _minMeshWidthThisTimeStep{std::numeric_limits<double>::infinity()};
  double _maxMeshWidthThisTimeStep{0.0};

  double _minTimeStepSize{std::numeric_limits<double>::infinity()};
  double _maxTimeStepSize{0.0};

  double _minTimeStepSizeThisTimeStep{std::numeric_limits<double>::infinity()};
  double _maxTimeStepSizeThisTimeStep{0.0};

  /**
   * Every traversal task's mergeLocalUpdates() may raise the flag concurrently.
   */
  std::atomic<bool> _hasHangingFacesThisTimeStep{false};

  bool _meshRefinementRanSinceLastTimeStep{false};
  bool _hasStoredFaceData{false};
};

#include "exahype2/Solver.cpph"
