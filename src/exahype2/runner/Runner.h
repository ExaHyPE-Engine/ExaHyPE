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

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

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
 * Entry points of the generated solver repository. Every rank calls every phase callback
 * collectively. Each callback reads the partition of the spacetree.
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

  std::function<void(tarch::mpi::ReductionRecord& record)>       addLocalValuesToGlobalReduction{};
  std::function<void(const tarch::mpi::ReductionRecord& record)> readValuesFromGlobalReduction{};

  std::function<const std::vector<peano4::RefinementRequest>&()> getRefinementRequests{};

  std::function<double()>           getMinTimeStamp{};
  std::function<double()>           getMaxTimeStamp{};
  std::function<std::uint64_t()>    getNumberOfSolverCells{};
  std::function<std::uint64_t()>    getNumberOfCellDoFs{};
  std::function<SolverStatistics()> getSolverStatistics{};

  std::function<ConfigurationSummary()> getConfigurationSummary{};
};

/**
 * Drives one simulation on every rank. Every rank runs the same steps. One allreduce at the end of
 * each step reduces the statistics, the values of the solvers and the rank of a non-critical
 * assertion. Every rank then takes the same decision on the next step. The grid construction runs
 * CreateGrid steps and a regrid runs DynamicAMR steps until the refinement requests of the solvers
 * leave the refinement plan unchanged. Every round of the grid initialisation runs one InitGrid
 * step.
 */
class exahype2::runner::Runner final {
public:
  /**
   * Program steps of a run.
   */
  enum class Step : int { CreateGrid = 0, AllocateMemory, InitGrid, PlotSolution, TimeStep, DynamicAMR };

  static int launch(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    const SolverRepositoryCallbacks&             callbacks,
    int&                                         argc,
    char**                                       argv
  );

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
   * Upper bound on the steps of the grid construction and of one regrid. A phase that reaches it
   * stops the run with an error.
   */
  static constexpr int MaximumMeshUpdateSteps = 1024;
  /**
   * Upper bound on the initialisation rounds. Refinement requests that still change the grid after
   * the last round stop the run with an error.
   */
  static constexpr int MaximumInitialisationRounds = 16;

  struct MeshUpdates {
    int          regrids{0};
    int          steps{0};
    double       wallClock{0.0};
    GridSnapshot before{};
    GridSnapshot after{};
  };

  struct StepTime {
    std::string                         name{};
    tarch::instrumentation::Measurement measurement{};
  };

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
   * Column of the values behind the labels of the time loop. It only grows. The values then stay
   * aligned.
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
   * Cell and degree-of-freedom updates of the measured time steps, summed over the solvers.
   */
  double _cellUpdates{0.0};
  double _dofUpdates{0.0};
  /**
   * Wall clock in seconds of the mesh updates of the grid initialisation. The estimate of the time
   * loop excludes it.
   */
  double _meshUpdateSecondsBeforeTimeLoop{0.0};
  bool   _stoppedAfterNonCriticalAssertion{false};

  /**
   * Time steps and their wall clock in seconds since the previous time step report.
   */
  unsigned long _stepsSinceReport{0};
  double        _stepSecondsSinceReport{0.0};
  /**
   * Cell and degree-of-freedom updates and wall clock in seconds of the measured time steps since
   * the previous time step report.
   */
  double _cellUpdatesSinceReport{0.0};
  double _dofUpdatesSinceReport{0.0};
  double _measuredSecondsSinceReport{0.0};

  double runStep(Step step);
  void executeStep(Step step);
  void finishStep(Step step);
  void regrid(bool dataLive);

  void reportConfiguration();
  void constructGrid();
  void initialiseGrid();
  [[nodiscard]] bool hasPendingMeshUpdates() const;
  void applyMeshUpdates();
  double runMeshUpdateStep(bool startsRegrid);
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
