// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/runner/Console.h"
#include "exahype2/runner/Statistics.h"
#include "peano4/RefinementRequest.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  struct Regrid;
} // namespace peano4

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace exahype2::runner {
  struct SolverRepositoryCallbacks;
  class Runner;

  using ConfigurationEntries = std::vector<std::pair<std::string, std::string>>;
  using ConfigurationSummary = std::vector<std::pair<std::string, ConfigurationEntries>>;
} // namespace exahype2::runner

namespace exahype2::runner::tests {
  class RunnerTest;
} // namespace exahype2::runner::tests

/**
 * The entry points of the generated solver repository the runner drives. Every phase callback
 * runs on every rank at the same time and reads the partition of the spacetree.
 */
struct exahype2::runner::SolverRepositoryCallbacks {
  std::function<std::string()> getCopyrightBanner{};

  std::function<void()>                                                          printApplicationHelp{};
  std::function<void(tarch::reader::JSONReader* reader, int& argc, char** argv)> loadApplicationParameters{};

  std::function<void()> startSimulation{};
  std::function<void()> finishSimulation{};

  std::function<void()>                  allocate{};
  std::function<tarch::AllocatedBytes()> getAllocatedBytes{};

  std::function<void()>                             requestRefinementForGridConstruction{};
  std::function<void()>                             initialise{};
  std::function<void()>                             performTimeStep{};
  std::function<void(const peano4::Regrid& regrid)> regrid{};
  std::function<void()>                             plot{};

  std::function<const std::vector<peano4::RefinementRequest>&()> getRefinementRequests{};

  std::function<double()>           getMinTimeStamp{};
  std::function<double()>           getMaxTimeStamp{};
  std::function<std::uint64_t()>    getNumberOfSolverCells{};
  std::function<std::uint64_t()>    getNumberOfCellDoFs{};
  std::function<SolverStatistics()> getSolverStatistics{};

  std::function<ConfigurationSummary()> getConfigurationSummary{};
};

/**
 * Drives one simulation on every rank. launch() parses the runtime parameters, initialises tarch
 * and Peano and runs the program: The global master reports the configuration, constructs the
 * grid, allocates the solver data, initialises the grid, runs the time loop with its plots and
 * regrids and reports the summary; it broadcasts every step it runs, and every other rank runs
 * the steps it receives until the master broadcasts the termination step. A sweep is one step of
 * the grid construction, the grid initialisation or a regrid; a regrid runs sweeps until the
 * refinement requests of the solvers change the refinement plan no further.
 */
class exahype2::runner::Runner final {
public:
  /**
   * The program steps the global master broadcasts; every rank runs the body of the step.
   */
  enum class Step : int { CreateGrid = 0, AllocateMemory, InitGrid, PlotSolution, TimeStep, DynamicAMR };

  /**
   * Step that ends the run on every rank.
   */
  static constexpr int TerminationStep = -2;
  /**
   * Step of a rank that leaves the choice to the global master.
   */
  static constexpr int NoStep = -1;

  static int launch(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    const SolverRepositoryCallbacks&             callbacks,
    int&                                         argc,
    char**                                       argv
  );
  static int exchangeStep(int stepOfThisRank);

  Runner(
    tarch::la::Vector<DIMENSIONS, double> domainOffset,
    tarch::la::Vector<DIMENSIONS, double> domainSize,
    std::bitset<DIMENSIONS>               periodicBoundaries,
    SolverRepositoryCallbacks             callbacks
  );
  Runner(const Runner&)            = delete;
  Runner(Runner&&)                 = delete;
  Runner& operator=(const Runner&) = delete;
  Runner& operator=(Runner&&)      = delete;
  ~Runner()                        = default;

  int run();

private:
  friend class exahype2::runner::tests::RunnerTest;

  /**
   * Wall-clock seconds between two progress reports of one regrid.
   */
  static constexpr double SecondsBetweenMeshUpdateReports = 5.0;
  /**
   * Bounds the sweeps of the grid construction and of one regrid; a phase that reaches this cap
   * without settling stops the run with an error.
   */
  static constexpr int MaximumMeshUpdateSweeps = 1024;
  /**
   * Bounds the initialisation rounds; a run whose refinement requests still change the grid after
   * the last round stops with an error, so the time loop never starts with mesh changes outstanding.
   */
  static constexpr int MaximumInitialisationRounds = 16;

  /**
   * Records the regrids a report covers, their sweeps and the grid snapshots around them.
   */
  struct MeshUpdates {
    int          regrids{0};
    int          sweeps{0};
    double       wallClock{0.0};
    GridSnapshot before{};
    GridSnapshot after{};
  };

  struct StepTime {
    std::string                         name{};
    tarch::instrumentation::Measurement measurement{};
  };

  /**
   * The sections of the time loop's report; a plot after a plot continues the open section.
   */
  enum class Section { None, TimeStep, MeshUpdates, Plot, Summary };

  static const char* getStepName(Step step);
  static std::string describeMeshUpdates(const MeshUpdates& updates);
  static std::string describeRepetitions(const std::string& phaseName, int count, int regrids);

  tarch::la::Vector<DIMENSIONS, double> _domainOffset;
  tarch::la::Vector<DIMENSIONS, double> _domainSize;
  std::bitset<DIMENSIONS>               _periodicBoundaries;
  SolverRepositoryCallbacks             _callbacks;
  Report                                _report{};
  Section                               _openSection{Section::None};
  /**
   * Column of the values behind the labels of the time loop; it only grows, so values stay aligned.
   */
  std::size_t _valueColumn{0};

  tarch::instrumentation::Measurement _timeStepMeasurement{};
  tarch::instrumentation::Measurement _plotMeasurement{};
  tarch::instrumentation::Measurement _meshUpdateMeasurement{};
  std::vector<StepTime>               _stepTimes{};

  MeshUpdates _meshUpdates{};
  bool        _meshUpdatesStarted{false};
  /**
   * Regrids of the whole run, the ones of the grid initialisation included.
   */
  int _numberOfRegrids{0};

  /**
   * Wall clock in seconds of every completed time step, the first one included.
   */
  double _timeSteppingSeconds{0.0};
  /**
   * Cells and degrees of freedom updated by the time steps the throughput measures, summed over
   * those steps and over the solvers.
   */
  double _cellUpdates{0.0};
  double _dofUpdates{0.0};
  /**
   * Wall clock in seconds the mesh updates of the grid initialisation took; the time loop's estimate excludes them.
   */
  double _meshUpdateSecondsBeforeTimeLoop{0.0};
  bool   _stoppedAfterNonCriticalAssertion{false};

  /**
   * Time steps and their wall clock in seconds since the previous time step report.
   */
  unsigned long _stepsSinceReport{0};
  double        _stepSecondsSinceReport{0.0};
  /**
   * Cells, degrees of freedom and wall clock in seconds of the measured time steps since the
   * previous time step report.
   */
  double _cellUpdatesSinceReport{0.0};
  double _dofUpdatesSinceReport{0.0};
  double _measuredSecondsSinceReport{0.0};

  double runStep(Step step);
  void executeStep(Step step);
  void finishStep(Step step);
  void regrid(bool dataLive);
  void runWorker();

  void reportConfiguration();
  void constructGrid();
  void initialiseGrid();
  [[nodiscard]] bool hasPendingMeshUpdates() const;
  void applyMeshUpdates();
  double runMeshUpdateSweep(bool startsRegrid);
  void reportMeshUpdates();
  void startMeshUpdates();
  void runTimeLoop();
  void reportSummary();

  void logTimeStep(
    double        minimumTimeStamp,
    double        maximumTimeStamp,
    double        terminalTime,
    unsigned long completedSteps,
    int           maximumSteps
  );
  void logPlot(int snapshot, int numberOfSnapshots, double timeStamp, double wallClockSeconds);
  void openSection(Section section);
  [[nodiscard]] std::string formatETA(double fraction) const;
  void widenValueColumn(std::size_t indentation, const std::string& label);
  void writeLabelled(std::size_t indentation, const std::string& label, const std::string& value);
};
