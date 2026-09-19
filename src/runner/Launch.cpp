// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Launch.h"

#include "peano4/Init.h"
#include "peano4/topology/TopologyManager.h"
#include "runner/GlobalStatistics.h"
#include "runner/RuntimeParameters.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/reader/JSONReader.h"

namespace {
  bool helpRequested(const int argc, char* const* const argv) {
    for (int index = 1; index < argc; ++index) {
      const auto argument = std::string(argv[index]);
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }
} // namespace

/**
 * Parses the merged runtime configuration, initialises tarch and Peano, runs
 * the engine, and shuts them down.
 */
int runner::launch(
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::bitset<DIMENSIONS>&               periodicBoundaries,
  const runner::RepositoryCallbacks&           callbacks,
  std::vector<runner::Step>                    steps,
  int&                                         argc,
  char**                                       argv
) {
  auto& parameters = runner::RuntimeParameters::getInstance();

  if (helpRequested(argc, argv)) {
    parameters.printUsage(std::filesystem::path(argc > 0 ? argv[0] : "").filename().string());
    if (callbacks.printApplicationHelp) {
      callbacks.printApplicationHelp();
    }
    return EXIT_SUCCESS;
  }

  // Configuration errors must stop the process before MPI starts.
  parameters.configure(argc, argv);

  // The runtime and application parsers use one reader, whose usage tracking identifies keys neither consumes.
  auto* const configReader         = parameters.getConfigReader();
  const auto  applicationParserRan = static_cast<bool>(callbacks.loadApplicationParameters);
  if (applicationParserRan) {
    callbacks.loadApplicationParameters(configReader, argc, argv);
  }

  // Application keys remain unused without an application parser and cannot be classified as errors.
  if (configReader != nullptr and applicationParserRan) {
    const auto unrecognisedKeys = configReader->getUnusedKeys();
    if (not unrecognisedKeys.empty()) {
      std::string joined;
      for (const auto& key : unrecognisedKeys) {
        joined += joined.empty() ? "" : ", ";
        joined += key;
      }
      tarch::terminateWithError(
        "Unrecognised keys in configuration file \"" + parameters.getConfigFilePath() + "\": " + joined
        + ". Run with --help to list the available runtime options."
      );
    }
  }

  // Each parser retains arguments for the next parser, so remaining arguments are unknown.
  if (argc > 1) {
    std::string unrecognised;
    for (int index = 1; index < argc; ++index) {
      if (not unrecognised.empty()) {
        unrecognised += ' ';
      }
      unrecognised += argv[index];
    }
    tarch::terminateWithError(
      "Unrecognised command-line arguments: " + unrecognised
      + ". Run with --help to list the available runtime options."
    );
  }

  // Configure the prefix before tarch initialisation can emit log messages.
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

  peano4::topology::TopologyManager::getInstance().enableDynamicLoadBalancing(not parameters.isLoadBalancingDisabled());

  const auto& outputDirectory = parameters.getOutputPath();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::error_code outputDirectoryError;
    std::filesystem::create_directories(outputDirectory, outputDirectoryError);
    if (outputDirectoryError) {
      tarch::terminateWithError(
        "Cannot create output directory \"" + outputDirectory + "\": " + outputDirectoryError.message()
      );
    }

    const auto      outputGitignore = std::filesystem::path(outputDirectory) / ".gitignore";
    std::error_code outputGitignoreError;
    const bool      outputGitignoreExists = std::filesystem::exists(outputGitignore, outputGitignoreError);
    if (outputGitignoreError) {
      tarch::terminateWithError(
        "Cannot inspect output ignore file \"" + outputGitignore.string() + "\": " + outputGitignoreError.message()
      );
    }
    if (not outputGitignoreExists) {
      std::ofstream gitignore(outputGitignore);
      if (not gitignore) {
        tarch::terminateWithError("Cannot create output ignore file \"" + outputGitignore.string() + "\".");
      }
      gitignore << "*\n";
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  tarch::instrumentation::Log::setOutputFile(outputDirectory + parameters.getExecutableName() + ".log");

  peano4::init(domainOffset, domainSize, periodicBoundaries);

  // Every rank schedules the same plan, so the first grid sweep builds the
  // complete base grid with each rank owning its contiguous curve segment.
  peano4::topology::TopologyManager::getInstance().scheduleRegularGridConstruction(parameters.getGridLevels());

  if (callbacks.startSimulation) {
    callbacks.startSimulation();
  }

  runner::Engine engine(domainOffset, domainSize, periodicBoundaries, callbacks, std::move(steps));

  int returnCode = EXIT_SUCCESS;
#pragma omp parallel default(none) shared(engine, returnCode)
  {
    // Every worker arms its own trap: The traversal tasks run on all threads, and the
    // floating-point environment is a per-thread state.
    const tarch::FloatingPointExceptions::ScopedTrap threadTrap(__FILE__, __LINE__);
#pragma omp barrier
#pragma omp master
    {
      returnCode = engine.run();
    }
#pragma omp barrier
  }

  // The last sweep's boundary messages and periodic pipeline may still be in
  // flight, and they reference container buffers, so they complete before any
  // teardown below frees those buffers.
  tarch::mpi::AbstractCommunicationManager::getInstance().finishPendingWork();
  tarch::mpi::AbstractCommunicationManager::getInstance().stop();

  if (callbacks.finishSimulation) {
    callbacks.finishSimulation();
  }
  callbacks.shutdownDataRepository();

  runner::shutdownGlobalStatistics();
  peano4::shutdown();
  launchTrap.reset();
  tarch::shutdown();
  return returnCode;
}
