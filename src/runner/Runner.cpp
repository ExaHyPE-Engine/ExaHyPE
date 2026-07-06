// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Runner.h"

#include "peano4/Init.h"
#include "peano4/parallel/Node.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/Orchestrator.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/instrumentation/Info.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/reader/JSONReader.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

// ===========================================================================
// Runner - the SPMD engine
// ===========================================================================
runner::Runner::Runner(
  tarch::la::Vector<DIMENSIONS, double> domainOffset,
  tarch::la::Vector<DIMENSIONS, double> domainSize,
  std::bitset<DIMENSIONS>               periodicBC,
  RepositoryCallbacks                   callbacks,
  std::vector<Step>                     steps
) :
  _domainOffset(domainOffset),
  _domainSize(domainSize),
  _periodicBC(periodicBC),
  _callbacks(std::move(callbacks)),
  _steps(std::move(steps)),
  _measurements(),
  _statisticsLog(_callbacks, _measurements, _domainOffset, _domainSize, _periodicBC) {
  _statisticsLog.registerDefaultProviders();
}

const runner::Step& runner::Runner::findStep(int programStep) const {
  for (const auto& step : _steps) {
    if (step.programStep == programStep) {
      return step;
    }
  }
  throw std::runtime_error(
    "runner::Runner: no step registered for Peano program step " + std::to_string(programStep) + "."
  );
}

void runner::Runner::executeStepBody(int programStep) const {
  tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
  findStep(programStep).body();
}

double runner::Runner::runStep(int programStep) {
  // Measure the whole step (broadcast, traversal, load-balancer update) so the global collectives
  // at both ends are counted.
  tarch::instrumentation::Watch watch("runner::Runner", "runStep", false, false);
  watch.start();

  peano4::parallel::Node::getInstance().setNextProgramStep(programStep);
  peano4::parallel::Node::getInstance().continueToRun();

  executeStepBody(programStep);

  // Refresh the load balancer after the step so the next decision sees current statistics.
  {
    // This is the load balancer's own post-sweep decision refresh, distinct from the LoadBalancing
    // program step (which is a grid sweep). Hence a separate region name.
    SCOREP_USER_REGION("LoadBalancerUpdate", SCOREP_USER_REGION_TYPE_COMMON)
    loadBalancer().update();
  }
  if (_callbacks.afterStep) {
    _callbacks.afterStep();
  }

  watch.stop();
  const double elapsed = watch.getCalendarTime();
  _statisticsLog.recordStepTime(findStep(programStep).name, elapsed);
  return elapsed;
}

void runner::Runner::runWorker() const {
  while (peano4::parallel::Node::getInstance().continueToRun()) {
    executeStepBody(peano4::parallel::Node::getInstance().getCurrentProgramStep());
    {
      // The load balancer's own post-sweep decision refresh, distinct from the LoadBalancing
      // program step.
      SCOREP_USER_REGION("LoadBalancerUpdate", SCOREP_USER_REGION_TYPE_COMMON)
      loadBalancer().update();
    }
    if (_callbacks.afterStep) {
      _callbacks.afterStep();
    }
  }
}

void runner::Runner::assertConsistentStepRegistryAcrossRanks() const {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    return;
  }

  unsigned long long localHash = 1469598103934665603ULL; // FNV-1a offset basis
  const auto         mix       = [&localHash](unsigned long long value) {
    localHash ^= value;
    localHash *= 1099511628211ULL; // FNV-1a prime
  };
  for (const auto& step : _steps) {
    mix(static_cast<unsigned long long>(step.programStep));
    for (const char character : step.name) {
      mix(static_cast<unsigned long long>(static_cast<unsigned char>(character)));
    }
    mix(0xffffffffULL);
  }

  unsigned long long minHash      = 0;
  unsigned long long maxHash      = 0;
  const MPI_Comm     communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  MPI_Allreduce(&localHash, &minHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, communicator);
  MPI_Allreduce(&localHash, &maxHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, communicator);

  if (minHash != maxHash) {
    throw std::runtime_error(
      "runner::Runner: the registered step set differs across MPI ranks. Every rank must register "
      "identical steps (same ids and names) so that a broadcast program step resolves everywhere."
    );
  }
#endif
}

int runner::Runner::run() {
  assertConsistentStepRegistryAcrossRanks();

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runWorker();
    return EXIT_SUCCESS;
  }

  RunContext context{_callbacks, RuntimeParameters::getInstance(), _domainSize, _measurements, _statisticsLog};

  _statisticsLog.logConfiguration();
  const bool terminatedGracefully = Orchestrator(_steps).run(*this, context);

  peano4::parallel::Node::getInstance().setNextProgramStep(peano4::parallel::Node::Terminate);
  peano4::parallel::Node::getInstance().continueToRun(); // Send terminate instruction to other ranks

  // Graceful end after a noncritical assertion: the final plot is already written, so skip the
  // summary.
  if (terminatedGracefully) {
    _statisticsLog.emitClosingBox("Run terminated gracefully after a noncritical assertion");
    return EXIT_FAILURE;
  }

  _statisticsLog.emitFinalSummary();
  _statisticsLog.emitClosingBox("Run terminated successfully");
  return EXIT_SUCCESS;
}

// ===========================================================================
// run() - the whole lifecycle in one call
// ===========================================================================
namespace {
  bool helpRequested(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
      const std::string argument = argv[index];
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }
} // namespace

int runner::run(
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::bitset<DIMENSIONS>&               periodicBC,
  RepositoryCallbacks                          callbacks,
  std::vector<Step>                            steps,
  int&                                         argc,
  char**                                       argv
) {
  auto& parameters = RuntimeParameters::getInstance();

  if (helpRequested(argc, argv)) {
    parameters.printUsage(std::filesystem::path(argc > 0 ? argv[0] : "").filename().string());
    if (callbacks.printApplicationHelp) {
      callbacks.printApplicationHelp();
    }
    return EXIT_SUCCESS;
  }

  // Validate all config before tarch::init, so a typo aborts before MPI starts.
  parameters.configure(argc, argv);

  // Framework and application share one reader; a key neither parser reads is a typo.
  tarch::reader::JSONReader* const configReader         = parameters.getConfigReader();
  const bool                       applicationParserRan = static_cast<bool>(callbacks.loadApplicationParameters);
  if (applicationParserRan) {
    callbacks.loadApplicationParameters(configReader, argc, argv);
  }

  // Skip the check without the app parser, else its keys look like typos.
  if (configReader != nullptr and applicationParserRan) {
    const std::vector<std::string> unrecognisedKeys = configReader->getUnusedKeys();
    if (not unrecognisedKeys.empty()) {
      std::string joined;
      for (const std::string& key : unrecognisedKeys) {
        joined += joined.empty() ? "" : ", ";
        joined += key;
      }
      throw std::runtime_error(
        "Unrecognised key(s) in configuration file \"" + parameters.getConfigFilePath() + "\": " + joined +
        ". Run with --help to list the available runtime options."
      );
    }
  }

  // Each parser leaves args it does not know in argv; what remains is unknown.
  if (argc > 1) {
    std::string unrecognised;
    for (int index = 1; index < argc; ++index) {
      if (not unrecognised.empty()) {
        unrecognised += ' ';
      }
      unrecognised += argv[index];
    }
    throw std::runtime_error(
      "Unrecognised command-line argument(s): " + unrecognised +
      ". Run with --help to list the available runtime options."
    );
  }

  const std::string outputDirectory = parameters.getOutputPath();
  std::error_code   outputDirectoryError;
  std::filesystem::create_directories(outputDirectory, outputDirectoryError);
  const std::filesystem::path outputGitignore = std::filesystem::path(outputDirectory) / ".gitignore";
  if (not std::filesystem::exists(outputGitignore)) {
    std::ofstream(outputGitignore) << "*\n";
  }
  tarch::instrumentation::Log::setOutputFile(outputDirectory + parameters.getExecutableName() + ".log");

  // Configure the log-line prefix from the verbose flag before anything can log.
  const bool verbose = parameters.isVerbose();
  tarch::instrumentation::Log::setLogTimeStamp(verbose);
  tarch::instrumentation::Log::setLogTimeStampHumanReadable(true);
  tarch::instrumentation::Log::setLogThreadName(verbose);
  tarch::instrumentation::Log::setLogScopeName(verbose);
  tarch::instrumentation::Log::setLogMessageType(true);

  tarch::init(
    &argc,
    &argv,
    {parameters.getMPITimeOutWarning(), parameters.getMPIDeadlockTimeOut(), parameters.isMPIDeadlockDetectionEnabled()}
  );

  tarch::instrumentation::Log::setLogMachineName(tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);

  peano4::init(domainOffset, domainSize, periodicBC);

  initLoadBalancer(periodicBC.count() > 0);

  callbacks.initDataRepository();
  if (callbacks.startSimulation) {
    callbacks.startSimulation();
  }

  Runner runnerInstance(domainOffset, domainSize, periodicBC, callbacks, std::move(steps));

  int returnCode = EXIT_SUCCESS;
#if defined(PEANO_WITH_OPENMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      returnCode = runnerInstance.run();
#if defined(PEANO_WITH_OPENMP)
    }
  }
#endif

  if (callbacks.finishSimulation) {
    callbacks.finishSimulation();
  }
  loadBalancer().shutdown();
  callbacks.shutdownDataRepository();

  peano4::shutdown();
  tarch::shutdown();
  return returnCode;
}
