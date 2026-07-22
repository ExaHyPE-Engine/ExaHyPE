// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Launch.h"

#include "peano4/Init.h"
#include "runner/LoadBalancer.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Init.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/reader/JSONReader.h"

static bool helpRequested(int argc, char* const* argv) {
  for (int index = 1; index < argc; ++index) {
    const auto argument = std::string(argv[index]);
    if (argument == "--help" or argument == "-h") {
      return true;
    }
  }
  return false;
}

/**
 * Parses merged runtime configuration, initialises tarch, Peano, the load
 * balancer, and the data repository, runs the engine, and shuts them down.
 */
int runner::launch(
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

  // Configuration errors must stop the process before MPI starts.
  parameters.configure(argc, argv);

  // The runtime and application parsers use one reader, whose usage tracking identifies keys neither consumes.
  const auto configReader         = parameters.getConfigReader();
  const auto applicationParserRan = static_cast<bool>(callbacks.loadApplicationParameters);
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
      throw std::runtime_error(
        "Unrecognised key(s) in configuration file \"" + parameters.getConfigFilePath() + "\": " + joined
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
    throw std::runtime_error(
      "Unrecognised command-line argument(s): " + unrecognised
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
    {parameters.getMPITimeoutWarning(), parameters.getMPIDeadlockTimeout(), parameters.isMPIDeadlockDetectionEnabled()}
  );

  tarch::instrumentation::Log::setLogMachineName(tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);

  tarch::plotter::FileWriter::setDoublePrecisionOutput(parameters.getPlotDoublePrecision());

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
#ifdef PEANO_WITH_MPI
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
  tarch::instrumentation::Log::setOutputFile(outputDirectory + parameters.getExecutableName() + ".log");

  peano4::init(domainOffset, domainSize, periodicBC);

  initLoadBalancer();

  callbacks.initDataRepository();
  if (callbacks.startSimulation) {
    callbacks.startSimulation();
  }

  Engine engine(domainOffset, domainSize, periodicBC, callbacks, std::move(steps));

  int returnCode = EXIT_SUCCESS;
#if defined(PEANO_WITH_OPENMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      returnCode = engine.run();
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
