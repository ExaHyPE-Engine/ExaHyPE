// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Measurement.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace runner {
  struct RepositoryCallbacks {
    std::function<void()> initDataRepository;
    std::function<void()> shutdownDataRepository;

    std::function<void()> afterStep;

    std::function<std::size_t()> getGlobalCellDataMemoryUsageInBytes;
    std::function<std::size_t()> getGlobalFaceDataMemoryUsageInBytes;
    std::function<std::size_t()> getGlobalVertexDataMemoryUsageInBytes;

    std::function<std::string()> getConfigurationInformation;
    std::function<std::string()> getCopyrightBanner;

    std::function<void()>                                 printApplicationHelp;
    std::function<void(const std::string&, int&, char**)> loadApplicationParameters;

    std::function<void()> startSimulation;
    std::function<void()> finishSimulation;

    std::function<bool()>                                             mayPlot;
    std::function<double()>                                           getMinTimeStamp;
    std::function<double()>                                           getMaxTimeStamp;
    std::function<int()>                                              getNumberOfDoFs;
    std::function<std::vector<std::pair<std::string, std::string>>()> getSolverStatistics;

    std::function<std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>()>
      getConfigurationSummary;
  };

  /** Wall-time accumulators the time loop needs for ETA, average and throughput. */
  struct MainMeasurements {
    tarch::instrumentation::Measurement timeStep;
    tarch::instrumentation::Measurement plot;
    tarch::instrumentation::Measurement gridSweep;
  };

  /** Format a wall-clock duration in seconds, e.g. "2.50 s". */
  std::string formatWallClock(double seconds);

  /** A unit-fraction (0..1) rendered as "[####------]", @p width characters wide. */
  std::string formatProgressBar(double fraction, int width = 10);

  /**
   * The human-readable label for a registered step name.
   */
  std::string displayLabel(const std::string& stepName);

  /**
   * Collects "label : value" entries grouped into named sections for one box, and
   * renders them with the labels aligned.
   */
  class StatisticsReport {
  public:
    void add(const std::string& sectionLabel, const std::string& label, const std::string& value);

    std::vector<std::string> renderContent() const;

  private:
    struct Section {
      std::string                                      title;
      std::vector<std::pair<std::string, std::string>> entries;
    };
    std::vector<Section> _sections;
    Section&             sectionFor(const std::string& title);
  };

  /** Fills one section of a report; registered once and called in registration order. */
  using StatisticsProvider = std::function<void(StatisticsReport&)>;

  class StatisticsLog {
  public:
    StatisticsLog(
      const RepositoryCallbacks&                   repositories,
      MainMeasurements&                            measurements,
      const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
      const tarch::la::Vector<DIMENSIONS, double>& domainSize,
      const std::bitset<DIMENSIONS>&               periodicBC
    );

    void registerDefaultProviders();

    void logConfiguration();

    /**
     * Render the running-simulation box. @p phase is the left-hand label (e.g. "Time Step 42",
     * "Plot 3", "Grid Construction"), followed by the step's wall-clock time @p wallClockSeconds;
     * @p progress is the right-hand string (e.g. "t = 0.013 / 0.2   [####------] 41%   ETA 0h 2m").
     * The body always carries Grid, Load Balancing and Memory; Solver and Performance are added
     * when
     * @p includeSolverSections is true (i.e. once the solvers are initialised and time stepping).
     */
    void emitSimulationBox(
      const std::string& phase,
      double             wallClockSeconds,
      const std::string& progress,
      bool               includeSolverSections
    );

    /** Accumulate an observer's wall time (by name) for the final summary. */
    void recordStepTime(const std::string& observerName, double elapsedSeconds);

    /** The final summary box: per-observer totals + time-loop averages. */
    void emitFinalSummary();

    /** Closing box holding @p message and a final "Goodbye" line. */
    void emitClosingBox(const std::string& message);

    /** "ETA 0h 2m 10s" (empty when not estimable). */
    std::string formatETA() const;

  private:
    const RepositoryCallbacks&            _repositories;
    MainMeasurements&                     _measurements;
    tarch::la::Vector<DIMENSIONS, double> _domainOffset;
    tarch::la::Vector<DIMENSIONS, double> _domainSize;
    std::bitset<DIMENSIONS>               _periodicBC;

    // Two ordered provider groups define the fixed row layout of the simulation box. The base
    // group (Grid, Load Balancing, Memory) is always rendered; the solver group (Solver,
    // Performance) is rendered only once time stepping has begun.
    std::vector<StatisticsProvider> _baseProviders;
    std::vector<StatisticsProvider> _solverProviders;

    // Per-observer wall-time totals for the final summary, keyed by name, first-seen order.
    std::vector<std::pair<std::string, tarch::instrumentation::Measurement>> _stepTimes;

    StatisticsReport buildSimulationReport(bool includeSolverSections);
    void emitBox(const std::vector<std::string>& headerLines, const std::vector<std::string>& bodyLines, char rule);
  };
} // namespace runner
