// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Runner.h"

#include "exahype2/runner/RuntimeParameters.h"
#include "peano4/Init.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/Version.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CPUFeatureDetection.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/StackSizeLimitDetection.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/Device.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/ScoreP.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"
#include "tarch/reader/JSONReader.h"
#include "tarch/tasking/Runtime.h"

#include <mpi.h>
#if defined(OPEN_MPI)
#include <mpi-ext.h>
#endif

namespace {
  /**
   * PhaseName holds the names of the program steps and the time loop in the reports and the Score-P
   * regions.
   */
  struct PhaseName final {
    static constexpr const char* GridConstruction   = "Grid Construction";
    static constexpr const char* AllocateMemory     = "Allocate Memory";
    static constexpr const char* GridInitialisation = "Grid Initialisation";
    static constexpr const char* Plot               = "Plot";
    static constexpr const char* TimeStep           = "Time Step";
    static constexpr const char* DynamicAMR         = "Dynamic AMR";
    static constexpr const char* TimeLoop           = "Time Loop";
  };

  bool isHelpRequested(const int argc, char* const* const argv) {
    for (int index = 1; index < argc; ++index) {
      const auto argument = std::string(argv[index]);
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }

  /**
   * Returns the point-to-point layer and the byte transports that the environment selects. Returns
   * the library default for a layer that the environment leaves unset.
   */
  std::string getMPITransport() {
    // The report runs once at start-up before the tasking runtime creates any thread.
    const auto* const pointToPointLayer = std::getenv("OMPI_MCA_pml"); // NOLINT(concurrency-mt-unsafe)
    const auto* const byteTransport     = std::getenv("OMPI_MCA_btl"); // NOLINT(concurrency-mt-unsafe)
    if (pointToPointLayer == nullptr and byteTransport == nullptr) {
      return "Library Default";
    }
    std::string description = pointToPointLayer != nullptr ? pointToPointLayer : "Library Default";
    if (byteTransport != nullptr) {
      description += std::string(" over ") + byteTransport;
    }
    return description;
  }

  /**
   * Returns the total and the free space of /dev/shm. The OpenMP runtime and the byte transports of
   * one node map their segments into /dev/shm.
   */
  std::string getMemoryFilesystemHeadroom() {
    std::error_code error;
    const auto      space = std::filesystem::space("/dev/shm", error);
    if (error) {
      return "Unavailable";
    }
    return "Free " + exahype2::runner::Format::getMemory(space.available) + " of "
      + exahype2::runner::Format::getMemory(space.capacity);
  }

  /**
   * Returns the GPU backend of the build and the number of devices of a GPU backend.
   */
  std::string getGPUBackendLabel() {
#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
    const std::string backend = "SYCL";
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
    const std::string backend = "HIP";
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
    const std::string backend = "CUDA";
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
    const std::string backend = "Kokkos";
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
    const std::string backend = "OMPT";
#else
    const std::string backend = "None";
#endif
#if defined(PEANO_WITH_GPU_BACKEND)
    const auto numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    return backend + ", " + exahype2::runner::Format::getNumber(numberOfDevices)
      + (numberOfDevices == 1 ? " Device" : " Devices");
#else
    return backend;
#endif
  }

  /**
   * Returns whether the MPI library accepts pointers into CUDA device memory.
   */
  std::string getGPUAwareMPILabel() {
#if defined(OPEN_MPI) and defined(MPIX_CUDA_AWARE_SUPPORT)
    return MPIX_Query_cuda_support() == 1 ? "CUDA-Aware" : "Not CUDA-Aware";
#elif defined(MPIX_GPU_SUPPORT_CUDA)
    int supported = 0;
    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &supported);
    return supported == 1 ? "CUDA-Aware" : "Not CUDA-Aware";
#else
    return "Unknown";
#endif
  }

  std::string getOnOff(const bool value) { return value ? "On" : "Off"; }

  std::string renderStatistics(const std::vector<exahype2::runner::SolverStatistic>& statistics) {
    std::ostringstream out;
    for (const auto& statistic : statistics) {
      if (not out.str().empty()) {
        out << " · ";
      }
      out << statistic.label << " = ";
      if (statistic.kind == exahype2::runner::SolverStatistic::Kind::Count) {
        out << statistic.count;
      } else if (statistic.minimum == statistic.maximum) {
        out << exahype2::runner::Format::getScientific(statistic.minimum);
      } else {
        out << "[" << exahype2::runner::Format::getScientific(statistic.minimum) << ", "
            << exahype2::runner::Format::getScientific(statistic.maximum) << "]";
      }
    }
    return out.str();
  }

  std::string getSeconds(const double value) {
    std::ostringstream result;
    result << std::fixed << std::setprecision(4) << value << " s";
    return result.str();
  }

  /**
   * Renders the rate of the cell updates. Adds the rate of the degree-of-freedom updates when the
   * solvers count any.
   */
  std::string getPerformanceOf(const double cellUpdates, const double dofUpdates, const double seconds) {
    assertion(seconds > 0.0 and cellUpdates >= 0.0 and dofUpdates >= 0.0, seconds, cellUpdates, dofUpdates);
    std::ostringstream performance;
    performance << exahype2::runner::Format::getRate(cellUpdates / seconds, "Cell");
    if (dofUpdates > 0.0) {
      performance << " · " << exahype2::runner::Format::getRate(dofUpdates / seconds, "DoF");
    }
    return performance.str();
  }
} // namespace

const char*
exahype2::runner::Runner::getStepName(const Step step) {
  switch (step) {
  case Step::CreateGrid:
    return PhaseName::GridConstruction;
  case Step::AllocateMemory:
    return PhaseName::AllocateMemory;
  case Step::InitGrid:
    return PhaseName::GridInitialisation;
  case Step::PlotSolution:
    return PhaseName::Plot;
  case Step::TimeStep:
    return PhaseName::TimeStep;
  case Step::DynamicAMR:
    return PhaseName::DynamicAMR;
  }
  tarch::terminateWithError("Each step of a run needs a phase name in the statistics.");
}

/**
 * Parses the merged runtime configuration and initialises tarch and Peano. Runs the program on
 * every rank. Shuts Peano and tarch down.
 */
int
exahype2::runner::Runner::launch(
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::bitset<DIMENSIONS>&               periodicBoundaries,
  const SolverRepositoryCallbacks&             callbacks,
  int&                                         argc,
  char**                                       argv
) {
  auto& parameters = exahype2::runner::RuntimeParameters::getInstance();
  if (isHelpRequested(argc, argv)) {
    parameters.printUsage(std::filesystem::path(argc > 0 ? argv[0] : "").filename().string());
    if (callbacks.printApplicationHelp) {
      callbacks.printApplicationHelp();
    }
    return EXIT_SUCCESS;
  }

  // A configuration error stops the process before MPI starts.
  parameters.configure(argc, argv);

  // The runtime and the application parser read one file. A key that neither reads is unrecognised.
  auto* const configReader         = parameters.getConfigReader();
  const auto  applicationParserRan = static_cast<bool>(callbacks.loadApplicationParameters);
  if (applicationParserRan) {
    callbacks.loadApplicationParameters(configReader, argc, argv);
  }
  if (configReader != nullptr and applicationParserRan) {
    const auto unrecognisedKeys = configReader->getUnusedKeys();
    if (not unrecognisedKeys.empty()) {
      std::string joined;
      for (const auto& key : unrecognisedKeys) {
        joined += joined.empty() ? "" : ", ";
        joined += key;
      }
      tarch::terminateWithError(
        "The configuration file '" + parameters.getConfigFilePath()
        + "' accepts the runtime options of --help only (got " + joined + ")."
      );
    }
  }
  // Every parser keeps the arguments it does not consume. The arguments left are unknown.
  if (argc > 1) {
    std::string unrecognised;
    for (int index = 1; index < argc; ++index) {
      if (not unrecognised.empty()) {
        unrecognised += ' ';
      }
      unrecognised += argv[index];
    }
    tarch::terminateWithError(
      "The command line accepts the runtime options of --help only (got " + unrecognised + ")."
    );
  }

  const auto verbose = parameters.isVerbose();
  tarch::instrumentation::Log::setLogTimeStamp(verbose);
  tarch::instrumentation::Log::setLogTimeStampHumanReadable(true);
  tarch::instrumentation::Log::setLogThreadName(verbose);
  tarch::instrumentation::Log::setLogScopeName(verbose);
  tarch::instrumentation::Log::setLogMessageType(true);

  tarch::init(
    &argc,
    &argv,
    {parameters.getMPITimeoutWarning(),
     parameters.getMPIDeadlockTimeout(),
     parameters.isMPIRequestTrackingEnabled(),
     parameters.isCommunicationThreadEnabled()}
  );
  std::optional<tarch::FloatingPointExceptions::ScopedTrap> launchTrap(std::in_place, __FILE__, __LINE__);
  tarch::instrumentation::Log::setLogMachineName(tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);
  tarch::plotter::FileWriter::setDoublePrecisionOutput(parameters.getPlotDoublePrecision());
  peano4::Spacetree::getInstance().enableDynamicLoadBalancing(not parameters.isLoadBalancingDisabled());

  const auto& outputDirectory = parameters.getOutputPath();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::error_code outputDirectoryError;
    std::filesystem::create_directories(outputDirectory, outputDirectoryError);
    if (outputDirectoryError) {
      tarch::terminateWithError(
        "The runner cannot create the output directory '" + outputDirectory + "' (" + outputDirectoryError.message()
        + ")."
      );
    }
    const auto      outputGitignore = std::filesystem::path(outputDirectory) / ".gitignore";
    std::error_code outputGitignoreError;
    const bool      outputGitignoreExists = std::filesystem::exists(outputGitignore, outputGitignoreError);
    if (outputGitignoreError) {
      tarch::terminateWithError(
        "The runner cannot check the file '" + outputGitignore.string() + "' (" + outputGitignoreError.message() + ")."
      );
    }
    if (not outputGitignoreExists) {
      std::ofstream gitignore(outputGitignore);
      if (not gitignore) {
        tarch::terminateWithError("The runner cannot create the file '" + outputGitignore.string() + "'.");
      }
      gitignore << "*\n";
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  tarch::instrumentation::Log::setOutputFile(outputDirectory + parameters.getExecutableName() + ".log");

  peano4::init(domainOffset, domainSize, periodicBoundaries);
  // Every rank schedules the same plan. The first grid step builds the base grid with one contiguous
  // curve segment per rank.
  peano4::Spacetree::getInstance().scheduleRegularGridConstruction(parameters.getGridLevels());

  if (callbacks.startSimulation) {
    callbacks.startSimulation();
  }

  Runner runner(domainOffset, domainSize, periodicBoundaries, callbacks);
  int    returnCode = EXIT_SUCCESS;
#pragma omp parallel default(none) shared(runner, returnCode)
  {
    // Every thread enables its own floating-point trap. The dispatches run on all threads. The
    // floating-point environment is a per-thread state.
    const tarch::FloatingPointExceptions::ScopedTrap threadTrap(__FILE__, __LINE__);
#pragma omp barrier
#pragma omp master
    {
      returnCode = runner.run();
    }
#pragma omp barrier
  }

  if (callbacks.finishSimulation) {
    callbacks.finishSimulation();
  }
  exahype2::runner::shutdownGlobalStatistics();
  peano4::shutdown();
  launchTrap.reset();
  tarch::shutdown();
  return returnCode;
}

/**
 * Returns the step of the global master. Every rank continues with this step. The same maximum
 * reduction sends the highest rank with a non-critical assertion to every rank.
 */
int
exahype2::runner::Runner::exchangeStep(const int stepOfThisRank) {
  assertion(stepOfThisRank >= TerminationStep, stepOfThisRank);
  constexpr int            NoDecision = TerminationStep - 1;
  const auto&              rank       = tarch::mpi::Rank::getInstance();
  const std::array<int, 2> localControl{
    rank.isGlobalMaster() ? stepOfThisRank : NoDecision,
    tarch::getNonCriticalAssertionRank()
  };
  std::array<int, 2> globalControl{};
  tarch::mpi::allreduce(localControl.data(), globalControl.data(), 2, MPI_INT, MPI_MAX, rank.getCommunicator());
  tarch::adoptNonCriticalAssertionRank(globalControl[1]);
  return globalControl[0];
}

exahype2::runner::Runner::Runner(
  const tarch::la::Vector<DIMENSIONS, double> domainOffset,
  const tarch::la::Vector<DIMENSIONS, double> domainSize,
  const std::bitset<DIMENSIONS>               periodicBoundaries,
  SolverRepositoryCallbacks                   callbacks
) :
  _domainOffset(domainOffset),
  _domainSize(domainSize),
  _periodicBoundaries(periodicBoundaries),
  _callbacks(std::move(callbacks)) {
  assertion(static_cast<bool>(_callbacks.requestRefinementForGridConstruction));
  assertion(static_cast<bool>(_callbacks.allocate));
  assertion(static_cast<bool>(_callbacks.initialise));
  assertion(static_cast<bool>(_callbacks.performTimeStep));
  assertion(static_cast<bool>(_callbacks.regrid));
  assertion(static_cast<bool>(_callbacks.plot));
  assertion(static_cast<bool>(_callbacks.getRefinementRequests));
}

/**
 * Applies the refinement requests of the solvers to the grid. With dataLive, the solvers move their
 * data to a changed grid before the regrid finishes.
 */
void
exahype2::runner::Runner::regrid(const bool dataLive) {
  auto&          spacetree = peano4::Spacetree::getInstance();
  peano4::Regrid regrid;
  const bool     changed = spacetree.regrid(_callbacks.getRefinementRequests(), regrid);
  if (changed and dataLive) {
    _callbacks.regrid(regrid);
    spacetree.finishRegrid();
  }
}

/**
 * Runs the body of one step on this rank.
 */
void
exahype2::runner::Runner::executeStep(const Step step) {
  const tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
  switch (step) {
  case Step::CreateGrid:
    _callbacks.requestRefinementForGridConstruction();
    regrid(false);
    break;
  case Step::AllocateMemory:
    _callbacks.allocate();
    break;
  case Step::InitGrid:
    _callbacks.initialise();
    break;
  case Step::PlotSolution:
    _callbacks.plot();
    break;
  case Step::TimeStep:
    _callbacks.performTimeStep();
    break;
  case Step::DynamicAMR:
    regrid(true);
    break;
  }
}

/**
 * Reduces the global statistics of the step. Every rank calls this function collectively after
 * every step. Every step except a time step refreshes the memory figures.
 */
void
exahype2::runner::Runner::finishStep(const Step step) {
  auto&      statistics = exahype2::runner::getGlobalStatistics();
  const auto bytes      = _callbacks.getAllocatedBytes ? _callbacks.getAllocatedBytes() : tarch::AllocatedBytes{};
  statistics.setLocalSolverBytes(bytes.host, bytes.device);
  statistics.setLocalCellUpdates(
    _callbacks.getNumberOfSolverCells ? _callbacks.getNumberOfSolverCells() : 0,
    _callbacks.getNumberOfCellDoFs ? _callbacks.getNumberOfCellDoFs() : 0
  );
  statistics.synchronise(step != Step::TimeStep);
}

/**
 * Broadcasts, executes and reduces one step. Returns its wall time in seconds. Only the global
 * master calls this function.
 */
double
exahype2::runner::Runner::runStep(const Step step) {
  tarch::instrumentation::Watch watch("exahype2::runner::Runner", "runStep", false, false);
  watch.start();
  exchangeStep(static_cast<int>(step));
  executeStep(step);
  finishStep(step);
  watch.stop();

  const auto elapsed = watch.getCalendarTime();
  const auto name    = std::string(getStepName(step));
  const auto record  = std::find_if(_stepTimes.begin(), _stepTimes.end(), [&name](const StepTime& stepTime) {
    return stepTime.name == name;
  });
  if (record == _stepTimes.end()) {
    _stepTimes.push_back({name, {}});
    _stepTimes.back().measurement.setValue(elapsed);
  } else {
    record->measurement.setValue(elapsed);
  }
  return elapsed;
}

void
exahype2::runner::Runner::runWorker() {
  for (int step = exchangeStep(NoStep); step != TerminationStep; step = exchangeStep(NoStep)) {
    executeStep(static_cast<Step>(step));
    finishStep(static_cast<Step>(step));
  }
}

/**
 * Returns the exit code of the run. A run that ended on a non-critical assertion fails.
 */
int
exahype2::runner::Runner::run() {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runWorker();
    return EXIT_SUCCESS;
  }
  reportConfiguration();
  constructGrid();
  {
    SCOREP_USER_REGION(PhaseName::AllocateMemory, SCOREP_USER_REGION_TYPE_PHASE)
    static_cast<void>(runStep(Step::AllocateMemory));
  }
  initialiseGrid();
  runTimeLoop();
  if (not _stoppedAfterNonCriticalAssertion) {
    reportSummary();
  }
  _report.rule();
  _report.line(
    _stoppedAfterNonCriticalAssertion ? "Run terminated gracefully after a non-critical assertion"
                                      : "Run terminated successfully"
  );
  _report.line("Goodbye");
  _report.close();
  // The workers wait inside exchangeStep(). The master broadcasts the termination step last.
  exchangeStep(TerminationStep);
  return _stoppedAfterNonCriticalAssertion ? EXIT_FAILURE : EXIT_SUCCESS;
}

/**
 * Writes the banners and the configuration report.
 */
void
exahype2::runner::Runner::reportConfiguration() {
  const auto& parameters = exahype2::runner::RuntimeParameters::getInstance();
  Table       report;
  if (not parameters.getProjectName().empty()) {
    report.add("Application", "Project", parameters.getProjectName());
  }
  if (not parameters.getExecutableName().empty()) {
    report.add("Application", "Executable", parameters.getExecutableName());
  }

  report.add("Build", "Dimensions", Format::getNumber(DIMENSIONS));
  report.add(
    "Build",
    "Space-Filling Curve",
    std::string(peano4::sfc::ActiveCurve::Name) + " (" + Format::getNumber(peano4::SubdivisionFactor)
      + " Children per Axis)"
  );
  const auto& rank          = tarch::mpi::Rank::getInstance();
  const auto  numberOfRanks = rank.getNumberOfRanks();
  const auto  numberOfNodes = rank.getNumberOfNodes();
  report.add(
    "Build",
    "MPI",
    Format::getNumber(numberOfRanks) + (numberOfRanks == 1 ? " Rank on " : " Ranks on ")
      + Format::getNumber(numberOfNodes) + (numberOfNodes == 1 ? " Node" : " Nodes")
  );
  const auto numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  report.add("Build", "OpenMP", Format::getNumber(numberOfThreads) + (numberOfThreads == 1 ? " Thread" : " Threads"));
  report.add("Build", "CPU Features", Format::getNumber(tarch::detectCPUFeatures()));
  report.add("Build", "Stack Size", Format::getNumber(tarch::detectStackSizeLimit()));
  report.add("Build", "GPU Backend", getGPUBackendLabel());
  report.add("Build", "GPU-Aware MPI", getGPUAwareMPILabel());
  report.add("Build", "Diagnostics", "Assertions=" + getOnOff(PEANO_ASSERTIONS) + " · Debug=" + getOnOff(PEANO_DEBUG));
  report.add("Build", "Git Branch", peano4::version::GitBranch);
  report.add("Build", "Git Commit", peano4::version::GitCommitHash);
  if (not parameters.getConfigFilePath().empty()) {
    report.add("Build", "Runtime Configuration", "JSON file \"" + parameters.getConfigFilePath() + "\"");
  }

  std::ostringstream domain;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    domain << (dimension == 0 ? "" : " x ") << "[" << _domainOffset[dimension] << ", "
           << (_domainOffset[dimension] + _domainSize[dimension]) << "]";
  }
  std::ostringstream periodic;
  constexpr char     AxisNames[]         = "XYZ";
  bool               hasPeriodicBoundary = false;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    if (_periodicBoundaries[dimension]) {
      periodic << (hasPeriodicBoundary ? ", " : "") << AxisNames[dimension];
      hasPeriodicBoundary = true;
    }
  }
  const auto         periodicAxes = periodic.str();
  std::ostringstream gridLevels;
  gridLevels << parameters.getGridLevels() << " .. " << parameters.getMaxGridLevels();
  std::ostringstream patchSizes;
  patchSizes << Format::getNumber(parameters.getMinAdmissiblePatchSize(_domainSize)) << " .. "
             << Format::getNumber(parameters.getMaxAdmissiblePatchSize(_domainSize));
  report.add("Domain & Grid", "Domain", domain.str());
  report.add("Domain & Grid", "Periodic Boundaries", periodicAxes.empty() ? "None" : periodicAxes);
  report.add("Domain & Grid", "Grid Levels", gridLevels.str());
  report.add("Domain & Grid", "Patch Size", patchSizes.str());

  report.add(
    "Time Stepping",
    "Terminal Time",
    RuntimeParameters::formatTerminalTime(parameters.getMinTerminalTime()) + " .. "
      + RuntimeParameters::formatTerminalTime(parameters.getMaxTerminalTime())
  );
  report.add(
    "Time Stepping",
    "Time Step Limit",
    parameters.getNumberOfTimeSteps() > 0 ? Format::getNumber(parameters.getNumberOfTimeSteps())
                                          : std::string("Unbounded")
  );
  report.add("Time Stepping", "First Plot", Format::getNumber(parameters.getFirstPlotTimeStamp()));
  report.add("Time Stepping", "Snapshots", Format::getNumber(parameters.getNumberOfSnapshots()));
  report.add("Time Stepping", "Plot Format", tarch::plotter::toString(parameters.getPlotFormat()));
  report.add("Time Stepping", "Output", parameters.getOutputPath());
  report.add("Time Stepping", "Log Interval", Format::getNumber(parameters.getTimeStepLogInterval()) + " s");

  report.add("MPI", "Transport", getMPITransport());
  report.add("MPI", "Memory Filesystem", getMemoryFilesystemHeadroom());
  report.add("MPI", "Communication Thread", getOnOff(tarch::mpi::AbstractCommunicationManager::usesDedicatedThread()));
  report.add("MPI", "Timeout Warning", Format::getNumber(parameters.getMPITimeoutWarning()) + " s");
  report.add("MPI", "Deadlock Timeout", Format::getNumber(parameters.getMPIDeadlockTimeout()) + " s");
  report.add("MPI", "Request Tracking", getOnOff(parameters.isMPIRequestTrackingEnabled()));
  report.add("MPI", "Dynamic Load Balancing", getOnOff(not parameters.isLoadBalancingDisabled()));

  if (_callbacks.getConfigurationSummary) {
    for (const auto& [section, entries] : _callbacks.getConfigurationSummary()) {
      for (const auto& [label, value] : entries) {
        report.add(section, label, value);
      }
    }
  }

  _report.line("Peano 4 (C) www.peano-framework.org");
  if (_callbacks.getCopyrightBanner) {
    _report.line(_callbacks.getCopyrightBanner());
  }
  _report.rule();
  for (const auto& bodyLine : report.render()) {
    _report.line(bodyLine);
  }
  _report.rule();
}

/**
 * Builds the base grid in the first step and the adaptive levels that the refinement criterion
 * asks for in further steps.
 */
void
exahype2::runner::Runner::constructGrid() {
  const auto& parameters = exahype2::runner::RuntimeParameters::getInstance();
  const auto& statistics = exahype2::runner::getGlobalStatistics();

  tarch::instrumentation::Watch constructionWatch("exahype2::runner::Runner", "constructGrid", false, false);
  constructionWatch.start();

  const auto coarsestAdmissiblePatchSize = parameters.getMaxAdmissiblePatchSize(_domainSize);
  auto       currentMinimumPatchSize     = std::numeric_limits<double>::max();
  bool       finishConditionSeenLastStep = false;
  bool       settled                     = false;
  int        steps                       = 0;
  while (not settled and steps < MaximumMeshUpdateSteps) {
    {
      SCOREP_USER_REGION(PhaseName::GridConstruction, SCOREP_USER_REGION_TYPE_COMMON)
      static_cast<void>(runStep(Step::CreateGrid));
    }
    steps++;
    if (tarch::la::smaller(statistics.getGlobalMinimumCellWidth(), currentMinimumPatchSize)) {
      currentMinimumPatchSize     = statistics.getGlobalMinimumCellWidth();
      finishConditionSeenLastStep = false;
    }
    // The construction settles when on two consecutive steps the finest cell reaches the coarsest
    // admissible size and the last regrid changes nothing.
    const bool converged = tarch::la::smallerEquals(currentMinimumPatchSize, coarsestAdmissiblePatchSize)
      and statistics.getGlobalNumberOfStationaryRegrids() >= 1;
    settled                     = converged and finishConditionSeenLastStep;
    finishConditionSeenLastStep = converged;
  }
  if (not settled) {
    tarch::terminateWithError(
      "The refinement criterion must stop changing the grid within " + std::to_string(MaximumMeshUpdateSteps)
      + " construction steps."
    );
  }
  constructionWatch.stop();
  const std::string summary = "Grid Constructed " + Format::getWallClock(constructionWatch.getCalendarTime()) + " · "
                            + describeRepetitions(PhaseName::GridConstruction, steps, 0);
  _report.line(std::string(BlockIndent, ' ') + summary);
}

/**
 * Initialises the solution and adapts the mesh before the time loop. Every round applies the
 * refinement votes in one regrid step and initialises the new grid again. Every cell then votes on
 * the analytic initial condition.
 */
void
exahype2::runner::Runner::initialiseGrid() {
  SCOREP_USER_REGION(PhaseName::GridInitialisation, SCOREP_USER_REGION_TYPE_PHASE)
  double elapsedSeconds = runStep(Step::InitGrid);
  // From the first initialisation on, a regrid keeps the solver data.
  peano4::Spacetree::getInstance().markPersistentDataLive();
  int rounds = 1;
  while (hasPendingMeshUpdates() and rounds < MaximumInitialisationRounds) {
    if (not _meshUpdatesStarted or _meshUpdates.steps == 0) {
      startMeshUpdates();
    }
    runMeshUpdateStep(true);
    elapsedSeconds += runStep(Step::InitGrid);
    rounds++;
  }
  if (hasPendingMeshUpdates()) {
    tarch::terminateWithError(
      "The refinement criterion must stop changing the grid within " + std::to_string(MaximumInitialisationRounds)
      + " initialisation rounds. It must not refine and erase the same cells in turn. It must ask for levels "
        "between -lmin and -lmax only."
    );
  }
  const std::string summary = "Grid Initialised " + Format::getWallClock(elapsedSeconds) + " · "
                            + describeRepetitions(PhaseName::GridInitialisation, rounds, 0);
  _report.line(std::string(BlockIndent, ' ') + summary);
  if (_meshUpdates.steps > 0) {
    _report.line(std::string(BlockIndent, ' ') + PhaseName::DynamicAMR + " " + describeMeshUpdates(_meshUpdates));
  }
  startMeshUpdates();
  exahype2::runner::writeGridSnapshot(_report);
  _report.rule();
}

/**
 * Returns whether the refinement requests of the solvers change the refinement plan. Every rank
 * returns the same answer.
 */
bool
exahype2::runner::Runner::hasPendingMeshUpdates() const {
  return peano4::Spacetree::getInstance().requestsChangePlan(_callbacks.getRefinementRequests());
}

/**
 * Renders the wall clock, the regrids and their steps since the previous report.
 */
std::string
exahype2::runner::Runner::describeMeshUpdates(const MeshUpdates& updates) {
  assertion(updates.regrids >= 0 and updates.steps >= updates.regrids, updates.regrids, updates.steps);
  return Format::getWallClock(updates.wallClock) + " · "
    + describeRepetitions(PhaseName::DynamicAMR, updates.steps, updates.regrids);
}

/**
 * Renders how often a phase ran in the unit of the phase. The unit is the plot, the regrid with its
 * steps, the round or the step.
 */
std::string
exahype2::runner::Runner::describeRepetitions(const std::string& phaseName, const int count, const int regrids) {
  assertion(count >= 0 and regrids >= 0, count, regrids);
  const auto getCountOf = [](const int number, const std::string& singular) {
    return Format::getGroupedNumber(static_cast<unsigned long>(number)) + " " + singular + (number == 1 ? "" : "s");
  };
  if (phaseName == PhaseName::Plot) {
    return getCountOf(count, "Plot");
  }
  if (phaseName == PhaseName::DynamicAMR) {
    return getCountOf(regrids, "Regrid") + " · " + getCountOf(count, "Step");
  }
  if (phaseName == PhaseName::GridInitialisation) {
    return getCountOf(count, "Round");
  }
  return getCountOf(count, "Step");
}

/**
 * Deletes the collected regrids and records the grid that the next report starts from.
 */
void
exahype2::runner::Runner::startMeshUpdates() {
  _meshUpdates        = MeshUpdates{};
  _meshUpdates.before = exahype2::runner::collectGridSnapshot();
  _meshUpdatesStarted = true;
}

/**
 * Writes the collected regrids behind a rule, followed by the grid and memory rows. Every arrow
 * runs from the value before the first regrid to the value after the last.
 */
void
exahype2::runner::Runner::reportMeshUpdates() {
  if (_meshUpdates.steps == 0) {
    return;
  }
  _meshUpdates.after = exahype2::runner::collectGridSnapshot();
  openSection(Section::MeshUpdates);
  writeLabelled(BlockIndent, PhaseName::DynamicAMR, describeMeshUpdates(_meshUpdates));
  for (const auto& line : exahype2::runner::describeGridTransition(_meshUpdates.before, _meshUpdates.after)) {
    _report.line(std::string(BlockIndent, ' ') + line);
  }
  startMeshUpdates();
}

/**
 * Applies the refinement requests of the solvers in one regrid. Its steps run until the requests
 * leave the refinement plan unchanged. A regrid reports the collected regrids every
 * SecondsBetweenMeshUpdateReports and before it stops the run.
 */
void
exahype2::runner::Runner::applyMeshUpdates() {
  SCOREP_USER_REGION(PhaseName::DynamicAMR, SCOREP_USER_REGION_TYPE_PHASE)
  if (not _meshUpdatesStarted or _meshUpdates.steps == 0) {
    startMeshUpdates();
  }
  int    stepsOfThisRegrid{0};
  double secondsSinceLastReport{0.0};
  while (hasPendingMeshUpdates() and stepsOfThisRegrid < MaximumMeshUpdateSteps) {
    // A report inside the regrid resets the collection. The regrid counts itself on its first step.
    secondsSinceLastReport += runMeshUpdateStep(stepsOfThisRegrid == 0);
    stepsOfThisRegrid++;
    if (secondsSinceLastReport >= SecondsBetweenMeshUpdateReports) {
      reportMeshUpdates();
      secondsSinceLastReport = 0.0;
    }
  }
  if (hasPendingMeshUpdates()) {
    reportMeshUpdates();
    std::ostringstream message;
    message << "The solvers must stop requesting mesh updates within " << MaximumMeshUpdateSteps
            << " dynamic AMR steps.";
    tarch::terminateWithError(message.str());
  }
}

/**
 * Runs one regrid step and adds it to the collected mesh updates. Returns its wall clock in
 * seconds. With startsRegrid, the step also counts a regrid.
 */
double
exahype2::runner::Runner::runMeshUpdateStep(const bool startsRegrid) {
  if (startsRegrid) {
    _meshUpdates.regrids++;
    _numberOfRegrids++;
  }
  const double elapsedSeconds = runStep(Step::DynamicAMR);
  assertion(elapsedSeconds >= 0.0, elapsedSeconds);
  _meshUpdates.wallClock += elapsedSeconds;
  _meshUpdateMeasurement.setValue(elapsedSeconds);
  _meshUpdates.steps++;
  return elapsedSeconds;
}

std::string
exahype2::runner::Runner::formatETA(const double fraction) const {
  const auto regridSeconds = _meshUpdateMeasurement.getAccumulatedValue() - _meshUpdateSecondsBeforeTimeLoop;
  const auto elapsed       = _timeSteppingSeconds + _plotMeasurement.getAccumulatedValue() + regridSeconds;
  if (elapsed <= 0.0 or fraction <= 0.0 or fraction >= 1.0) {
    return "";
  }
  const double remainingSeconds = elapsed * (1.0 - fraction) / fraction;
  if (remainingSeconds >= std::numeric_limits<double>::max() / 10.0) {
    return "";
  }
  return Format::getDuration(remainingSeconds);
}

/**
 * Moves the value column right of the label. No label of the time loop reaches its values.
 */
void
exahype2::runner::Runner::widenValueColumn(const std::size_t indentation, const std::string& label) {
  _valueColumn = std::max(_valueColumn, indentation + getDisplayWidth(label) + 2);
}

/**
 * Writes the label at the indentation and the value at the value column. A broken value continues
 * at the value column.
 */
void
exahype2::runner::Runner::writeLabelled(
  const std::size_t  indentation,
  const std::string& label,
  const std::string& value
) {
  widenValueColumn(indentation, label);
  _report.line(padToWidth(std::string(indentation, ' ') + label, _valueColumn) + value, _valueColumn);
}

/**
 * Writes the time step report with the average step time, one line per solver and the throughput
 * since the previous report. The value column widens for every label before the first line.
 */
void
exahype2::runner::Runner::logTimeStep(
  const double        minimumTimeStamp,
  const double        maximumTimeStamp,
  const double        terminalTime,
  const unsigned long completedSteps,
  const int           maximumSteps
) {
  assertion(completedSteps > 0 and _stepsSinceReport > 0, completedSteps, _stepsSinceReport);
  [[maybe_unused]] const bool timeStampIsFinite = std::isfinite(minimumTimeStamp);
  assertion(timeStampIsFinite and minimumTimeStamp <= maximumTimeStamp, minimumTimeStamp, maximumTimeStamp);
  reportMeshUpdates();

  const bool timeBounded = terminalTime > 0.0 and terminalTime < std::numeric_limits<double>::max() / 10.0;
  double     fraction    = -1.0;
  if (maximumSteps > 0) {
    fraction = std::clamp(static_cast<double>(completedSteps) / maximumSteps, 0.0, 1.0);
  } else if (timeBounded) {
    // The minimum stamp is the time the whole domain has reached.
    fraction = std::clamp(minimumTimeStamp / terminalTime, 0.0, 1.0);
  }
  const std::string estimatedTimeRemaining = fraction > 0.0 ? formatETA(fraction) : "";

  SolverStatistics solvers;
  if (_callbacks.getSolverStatistics) {
    solvers = _callbacks.getSolverStatistics();
  }

  std::string label = std::string(PhaseName::TimeStep) + " " + Format::getGroupedNumber(completedSteps);
  if (maximumSteps > 0) {
    label += "/" + Format::getGroupedNumber(static_cast<unsigned long>(maximumSteps));
  }
  widenValueColumn(BlockIndent, label);
  widenValueColumn(2 * BlockIndent, "Performance");
  for (const auto& [name, statistics] : solvers) {
    widenValueColumn(2 * BlockIndent, name);
  }

  std::ostringstream timeStep;
  timeStep << Format::getWallClock(_stepSecondsSinceReport / static_cast<double>(_stepsSinceReport))
           << " · t = " << Format::getScientific(minimumTimeStamp);
  // Local time stepping spans a range of stamps. Global time stepping gives every cell one stamp.
  if (maximumTimeStamp > minimumTimeStamp) {
    timeStep << " .. " << Format::getScientific(maximumTimeStamp);
  }
  if (timeBounded) {
    timeStep << " / " << Format::getScientific(terminalTime);
  }
  if (fraction >= 0.0) {
    timeStep << " · " << std::lround(fraction * 100.0) << " %";
    if (not estimatedTimeRemaining.empty()) {
      timeStep << " · ETA " << estimatedTimeRemaining;
    }
  }

  openSection(Section::TimeStep);
  writeLabelled(BlockIndent, label, timeStep.str());
  for (const auto& [name, statistics] : solvers) {
    writeLabelled(2 * BlockIndent, name, renderStatistics(statistics));
  }
  if (_measuredSecondsSinceReport > 0.0) {
    writeLabelled(
      2 * BlockIndent,
      "Performance",
      getPerformanceOf(_cellUpdatesSinceReport, _dofUpdatesSinceReport, _measuredSecondsSinceReport)
    );
  }
  _stepsSinceReport           = 0;
  _stepSecondsSinceReport     = 0.0;
  _cellUpdatesSinceReport     = 0.0;
  _dofUpdatesSinceReport      = 0.0;
  _measuredSecondsSinceReport = 0.0;
}

/**
 * Opens a section of the report behind a rule. A plot after a plot continues the open section.
 */
void
exahype2::runner::Runner::openSection(const Section section) {
  if (section != Section::Plot or _openSection != Section::Plot) {
    _report.rule();
  }
  _openSection = section;
}

/**
 * Writes the report line of one plot. The line of the initial condition omits the snapshot number.
 */
void
exahype2::runner::Runner::logPlot(
  const int    snapshot,
  const int    numberOfSnapshots,
  const double timeStamp,
  const double wallClockSeconds
) {
  [[maybe_unused]] const bool timeStampIsFinite = std::isfinite(timeStamp);
  assertion(wallClockSeconds >= 0.0 and timeStampIsFinite, wallClockSeconds, timeStamp);
  std::ostringstream label;
  if (snapshot <= 0) {
    label << "Initial Plot";
  } else {
    label << "Plot " << snapshot;
    if (numberOfSnapshots > 0) {
      label << "/" << numberOfSnapshots;
    }
  }
  openSection(Section::Plot);
  writeLabelled(
    BlockIndent,
    label.str(),
    Format::getWallClock(wallClockSeconds) + " · t = " + Format::getScientific(timeStamp)
  );
}

/**
 * Alternates time steps, plots and regrids until the terminal time or the step budget. The
 * refinement requests of a time step change the grid before the next plot or time step. After a
 * non-critical assertion, the loop plots the final state on the current grid and stops.
 */
void
exahype2::runner::Runner::runTimeLoop() {
  SCOREP_USER_REGION(PhaseName::TimeLoop, SCOREP_USER_REGION_TYPE_PHASE)
  const auto& parameters = exahype2::runner::RuntimeParameters::getInstance();

  _meshUpdateSecondsBeforeTimeLoop = _meshUpdateMeasurement.getAccumulatedValue();

  const auto    firstPlotTimeStamp = parameters.getFirstPlotTimeStamp();
  double        nextMinPlot        = firstPlotTimeStamp;
  double        nextMaxPlot        = firstPlotTimeStamp;
  double        nextPlotAtStep     = 0.0;
  unsigned long completedTimeSteps = 0;
  int           completedPlots     = 0;
  // lastLoggedStep holds the last reported step. The shutdown reports it once.
  unsigned long lastLoggedStep    = 0;
  bool          justWroteSnapshot = false;
  auto          lastReport        = std::chrono::steady_clock::now();

  const int  maxTimeSteps = parameters.getNumberOfTimeSteps();
  const bool stepLimited  = maxTimeSteps > 0;

  // The value column leaves room for step labels up to ReservedSteps. It moves in longer runs only.
  constexpr unsigned long ReservedSteps = 999999;
  widenValueColumn(
    BlockIndent,
    std::string(PhaseName::TimeStep) + " "
      + Format::getGroupedNumber(stepLimited ? static_cast<unsigned long>(maxTimeSteps) : ReservedSteps)
      + (stepLimited ? "/" + Format::getGroupedNumber(static_cast<unsigned long>(maxTimeSteps)) : "")
  );
  widenValueColumn(BlockIndent, PhaseName::DynamicAMR);
  if (_callbacks.getSolverStatistics) {
    for (const auto& [name, statistics] : _callbacks.getSolverStatistics()) {
      widenValueColumn(2 * BlockIndent, name);
    }
  }

  enum class Action { TimeStep, Plot, Stop };
  while (true) {
    if (hasPendingMeshUpdates() and not tarch::hasNonCriticalAssertionBeenViolated()) {
      applyMeshUpdates();
    }
    const double minTerminal      = parameters.getMinTerminalTime();
    const double maxTerminal      = parameters.getMaxTerminalTime();
    const double plotEnd          = maxTerminal < std::numeric_limits<double>::max() / 10.0 ? maxTerminal : minTerminal;
    const int    snapshots        = parameters.getNumberOfSnapshots();
    const double snapshotInterval = snapshots <= 0 ? 0.0 : plotEnd / snapshots;
    // With a step budget, the snapshots spread evenly over the steps.
    const double stepPlotInterval =
      (stepLimited and snapshots > 0) ? static_cast<double>(maxTimeSteps) / snapshots : 0.0;

    const double minStamp             = _callbacks.getMinTimeStamp();
    const double maxStamp             = _callbacks.getMaxTimeStamp();
    const bool   firstPlotTimeReached = minStamp >= firstPlotTimeStamp or maxStamp >= firstPlotTimeStamp;
    if (stepLimited and not firstPlotTimeReached) {
      nextPlotAtStep = static_cast<double>(completedTimeSteps) + 1.0;
    }
    const bool keepTimeStepping = stepLimited ? completedTimeSteps < static_cast<unsigned long>(maxTimeSteps)
                                              : (minStamp < minTerminal and maxStamp < maxTerminal);

    Action action;
    if (tarch::hasNonCriticalAssertionBeenViolated()) {
      if (not _stoppedAfterNonCriticalAssertion) {
        _stoppedAfterNonCriticalAssertion = true;
        logError("A non-critical assertion fails. The runner writes the final snapshot and stops.");
        action = Action::Plot;
      } else {
        action = Action::Stop;
      }
    } else if (
      stepLimited and stepPlotInterval > 0.0 and keepTimeStepping and firstPlotTimeReached
      and static_cast<double>(completedTimeSteps) >= nextPlotAtStep
    ) {
      nextPlotAtStep += stepPlotInterval;
      if (nextPlotAtStep <= static_cast<double>(completedTimeSteps)) {
        logWarning("The runner writes at most one snapshot per time step.");
        nextPlotAtStep = static_cast<double>(completedTimeSteps) + 1.0;
      }
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (
      not stepLimited and snapshotInterval > 0.0 and keepTimeStepping and completedPlots < snapshots - 1
      and (minStamp >= nextMinPlot or maxStamp >= nextMaxPlot)
    ) {
      // The final plot at termination completes the numbered snapshots. The interval trigger stops
      // one short. minStamp belongs to the slowest solver and maxStamp to the fastest. The fastest
      // solver sets the snapshot spacing.
      nextMinPlot += snapshotInterval;
      nextMaxPlot += snapshotInterval;
      if (nextMinPlot < minStamp) {
        logWarning("The runner writes at most one snapshot per time step.");
        nextMinPlot = minStamp + snapshotInterval;
      } else if (nextMaxPlot < maxStamp) {
        logWarning("The runner writes at most one snapshot per time step.");
        nextMaxPlot = maxStamp + snapshotInterval;
      }
      nextMaxPlot       = std::max(nextMaxPlot, nextMinPlot);
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (keepTimeStepping) {
      justWroteSnapshot = false;
      action            = Action::TimeStep;
    } else if (not justWroteSnapshot and (snapshotInterval > 0.0 or stepPlotInterval > 0.0)) {
      justWroteSnapshot = true;
      nextMinPlot       = std::numeric_limits<double>::max();
      nextMaxPlot       = std::numeric_limits<double>::max();
      nextPlotAtStep    = std::numeric_limits<double>::max();
      action            = Action::Plot;
    } else {
      action = Action::Stop;
    }

    if (action == Action::Stop) {
      // After a non-critical assertion, the report ends with the final plot and no time step summary.
      if (not _stoppedAfterNonCriticalAssertion and completedTimeSteps > 0 and completedTimeSteps != lastLoggedStep) {
        logTimeStep(
          _callbacks.getMinTimeStamp(),
          _callbacks.getMaxTimeStamp(),
          minTerminal,
          completedTimeSteps,
          maxTimeSteps
        );
      } else {
        reportMeshUpdates();
      }
      break;
    }

    if (action == Action::Plot) {
      double elapsedSeconds = 0.0;
      {
        SCOREP_USER_REGION(PhaseName::Plot, SCOREP_USER_REGION_TYPE_COMMON)
        elapsedSeconds = runStep(Step::PlotSolution);
      }
      _plotMeasurement.setValue(elapsedSeconds);
      // The plot of the initial condition precedes every step. It is not one of the numbered snapshots.
      if (completedTimeSteps == 0) {
        logPlot(0, 0, _callbacks.getMinTimeStamp(), elapsedSeconds);
      } else {
        ++completedPlots;
        logPlot(completedPlots, snapshots, _callbacks.getMinTimeStamp(), elapsedSeconds);
      }
      continue;
    }

    double elapsedSeconds = 0.0;
    {
      SCOREP_USER_REGION(PhaseName::TimeStep, SCOREP_USER_REGION_TYPE_COMMON)
      elapsedSeconds = runStep(Step::TimeStep);
    }
    _timeSteppingSeconds += elapsedSeconds;
    _stepsSinceReport++;
    _stepSecondsSinceReport += elapsedSeconds;
    // The first step warms the caches and the runtime. The throughput excludes it.
    if (completedTimeSteps > 0) {
      const auto& statistics = exahype2::runner::getGlobalStatistics();
      const auto  cells      = static_cast<double>(statistics.getGlobalNumberOfSolverCells());
      const auto  cellDoFs   = static_cast<double>(statistics.getGlobalNumberOfCellDoFs());
      _timeStepMeasurement.setValue(elapsedSeconds);
      _cellUpdates += cells;
      _dofUpdates += cellDoFs;
      _cellUpdatesSinceReport += cells;
      _dofUpdatesSinceReport += cellDoFs;
      _measuredSecondsSinceReport += elapsedSeconds;
    }
    ++completedTimeSteps;

    // A step with a non-critical assertion leaves invalid data. The next iteration plots the final
    // state without a report of the step.
    const auto now = std::chrono::steady_clock::now();
    if (
      not tarch::hasNonCriticalAssertionBeenViolated()
      and (lastLoggedStep == 0 or std::chrono::duration<double>(now - lastReport).count() >= parameters.getTimeStepLogInterval())
    ) {
      logTimeStep(
        _callbacks.getMinTimeStamp(),
        _callbacks.getMaxTimeStamp(),
        minTerminal,
        completedTimeSteps,
        maxTimeSteps
      );
      lastLoggedStep = completedTimeSteps;
      lastReport     = now;
    }
  }
}

/**
 * Writes the grid snapshot behind a rule. Writes the wall clock of every step kind and the
 * throughput of the time steps below it.
 */
void
exahype2::runner::Runner::reportSummary() {
  openSection(Section::Summary);
  exahype2::runner::writeGridSnapshot(_report);

  constexpr std::array<const char*, 6> StepOrder = {
    PhaseName::GridConstruction,
    PhaseName::AllocateMemory,
    PhaseName::GridInitialisation,
    PhaseName::Plot,
    PhaseName::DynamicAMR,
    PhaseName::TimeStep
  };
  const auto stepPosition = [&StepOrder](const std::string& name) {
    const auto* const position = std::find(StepOrder.begin(), StepOrder.end(), name);
    return std::distance(StepOrder.begin(), position);
  };
  auto stepTimes = _stepTimes;
  std::stable_sort(stepTimes.begin(), stepTimes.end(), [&stepPosition](const StepTime& left, const StepTime& right) {
    return stepPosition(left.name) < stepPosition(right.name);
  });

  constexpr const char* TimeSpent = "Time Spent";
  Table                 timeSpent;
  for (const auto& [name, measurement] : stepTimes) {
    std::ostringstream value;
    value << getSeconds(measurement.getAccumulatedValue());
    if (name != PhaseName::AllocateMemory) {
      value << "  (" << describeRepetitions(name, measurement.getNumberOfMeasurements(), _numberOfRegrids) << ")";
    }
    timeSpent.add(TimeSpent, name, value.str());
  }
  if (_timeStepMeasurement.getNumberOfMeasurements() > 0) {
    // The averages leave out the first time step. The first step warms the caches and the runtime.
    timeSpent.add(
      TimeSpent,
      "Average Time Step",
      getSeconds(_timeStepMeasurement.getAverageValue()) + "  ("
        + describeRepetitions(PhaseName::TimeStep, _timeStepMeasurement.getNumberOfMeasurements(), 0) + ")"
    );
    // The grid grows and shrinks. The rates divide the cells and the degrees of freedom of every
    // measured step by their time.
    const auto measuredSeconds = _timeStepMeasurement.getAccumulatedValue();
    if (measuredSeconds > 0.0) {
      timeSpent.add(TimeSpent, "Average Performance", getPerformanceOf(_cellUpdates, _dofUpdates, measuredSeconds));
    }
  }
  for (const auto& line : timeSpent.render()) {
    _report.line(line);
  }
}
