// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Launch.h"

#include "peano4/Init.h"
#include "runner/LoadBalancer.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Init.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/reader/JSONReader.h"

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

  // Validate all config before tarch::init, so a typo aborts before MPI starts.
  parameters.configure(argc, argv);

  // Framework and application share one reader; a key neither parser reads is a typo.
  const auto configReader         = parameters.getConfigReader();
  const auto applicationParserRan = static_cast<bool>(callbacks.loadApplicationParameters);
  if (applicationParserRan) {
    callbacks.loadApplicationParameters(configReader, argc, argv);
  }

  // Skip the check without the app parser, else its keys look like typos.
  if (configReader != nullptr and applicationParserRan) {
    const auto unrecognisedKeys = configReader->getUnusedKeys();
    if (not unrecognisedKeys.empty()) {
      std::string joined;
      for (const auto& key : unrecognisedKeys) {
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
      ". Run with --help to list the available runtime "
      "options."
    );
  }

  const auto      outputDirectory = parameters.getOutputPath();
  std::error_code outputDirectoryError;
  std::filesystem::create_directories(outputDirectory, outputDirectoryError);
  const auto outputGitignore = std::filesystem::path(outputDirectory) / ".gitignore";
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
