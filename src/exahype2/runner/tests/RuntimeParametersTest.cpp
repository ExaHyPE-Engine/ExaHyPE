// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParametersTest.h"

#include "exahype2/runner/RuntimeParameters.h"
#include "peano4/Partition.h"
#include "tarch/Termination.h"
#include "tarch/la/Vector.h"
#include "tarch/plotter/Format.h"
#include "tarch/reader/JSONReader.h"
#include "tarch/tests/TestCase.h"

namespace {
  struct Storage final {
    double                       speed{1.5};
    int                          count{2};
    bool                         verbose{false};
    std::string                  name{};
    tarch::la::Vector<2, double> offset{0.0};
    tarch::la::Vector<3, bool>   flags{false};
  };

  exahype2::runner::ParameterTable getTableOf(Storage& storage) {
    exahype2::runner::ParameterTable table;
    table.add("speed", &storage.speed);
    table.add("count", &storage.count);
    table.add("verbose", &storage.verbose);
    table.add("name", &storage.name);
    table.addArray("offset", storage.offset.getData(), 2);
    table.addArray("flags", storage.flags.getData(), 3);
    return table;
  }

  /**
   * The file name carries a per-run identifier, so two processes in the test directory never overwrite each other's
   * fixture. The caller deletes the file after reading it.
   */
  std::string writeConfiguration(const std::string& content) {
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int>     counter = 0;
    const std::filesystem::path path    = tarch::tests::getWritableTestDirectory()
      / std::filesystem::path("parameter-table-" + testRunIdentifier + "-" + std::to_string(counter++) + ".json");
    std::ofstream output(path);
    if (not output.is_open()) {
      tarch::terminateWithError("Failed to create temporary file \"" + path.string() + "\".");
    }
    output << content;
    return path.string();
  }

  struct Arguments final {
    explicit Arguments(std::initializer_list<std::string> arguments) :
      _arguments(arguments) {
      for (auto& argument : _arguments) {
        argv.push_back(argument.data());
      }
      argv.push_back(nullptr);
      argc = static_cast<int>(_arguments.size());
    }

    int                argc{0};
    std::vector<char*> argv{};

  private:
    std::vector<std::string> _arguments;
  };
} // namespace

exahype2::runner::tests::RuntimeParametersTest::RuntimeParametersTest() :
  tarch::tests::TestCase("exahype2::runner::tests::RuntimeParametersTest") {}

void
exahype2::runner::tests::RuntimeParametersTest::run() {
  testMethod(testNumbersParseFromTheWholeArgument);
  testMethod(testTheOutputPathAlwaysEndsInASeparator);
  testMethod(testTheMeshSizeDividesOncePerLevel);
  testMethod(testDefaultsReadBackAsTheyWereGiven);
  testMethod(testBothSpellingsOfTheLoadBalancingSwitchAreConsumed);
  testMethod(testTheMaximumTerminalTimeBoundsTheMinimumOne);
  testMethod(testThePlotFormatComesFromTheCommandLine);
  testMethod(testTheTableReadsTheCommandLineAndKeepsTheOtherArguments);
  testMethod(testTheTableReadsTheConfigurationFileBeforeTheCommandLine);
  testMethod(testTheTableUsageNamesEveryEntryWithItsDefault);
  testMethod(testTheConfigurationFileSetsTheEngineParameters);
}

void
exahype2::runner::tests::RuntimeParametersTest::testNumbersParseFromTheWholeArgument() {
  validateEquals(exahype2::runner::RuntimeParameters::parseInteger("42", "-lmax"), 42);
  validateEquals(exahype2::runner::RuntimeParameters::parseInteger("-7", "-lmax"), -7);
  validateNumericalEquals(exahype2::runner::RuntimeParameters::parseDouble("0.25", "-et"), 0.25);
  validateNumericalEquals(exahype2::runner::RuntimeParameters::parseDouble("-1.5e2", "-et"), -150.0);
}

void
exahype2::runner::tests::RuntimeParametersTest::testTheOutputPathAlwaysEndsInASeparator() {
  validateEquals(exahype2::runner::RuntimeParameters::normaliseOutputPath("solutions"), std::string("solutions/"));
  validateEquals(exahype2::runner::RuntimeParameters::normaliseOutputPath("solutions/"), std::string("solutions/"));
  validateEquals(exahype2::runner::RuntimeParameters::normaliseOutputPath("/tmp/a/b"), std::string("/tmp/a/b/"));
  validateEquals(exahype2::runner::RuntimeParameters::normaliseOutputPath(""), std::string("./"));
}

void
exahype2::runner::tests::RuntimeParametersTest::testTheMeshSizeDividesOncePerLevel() {
  constexpr double SubdivisionFactor = peano4::SubdivisionFactor;
  validateNumericalEquals(exahype2::runner::RuntimeParameters::divideBySubdivisionFactorPower(1.0, 0), 1.0);
  validateNumericalEquals(
    exahype2::runner::RuntimeParameters::divideBySubdivisionFactorPower(1.0, 1),
    1.0 / SubdivisionFactor
  );
  validateNumericalEquals(
    exahype2::runner::RuntimeParameters::divideBySubdivisionFactorPower(SubdivisionFactor * SubdivisionFactor, 2),
    1.0
  );
}

void
exahype2::runner::tests::RuntimeParametersTest::testDefaultsReadBackAsTheyWereGiven() {
  using RuntimeParameters = exahype2::runner::RuntimeParameters;
  validateEquals(RuntimeParameters::formatDefault(7), std::string("7"));
  validateEquals(RuntimeParameters::parseInteger(RuntimeParameters::formatDefault(7), "-key"), 7);
  validateNumericalEquals(RuntimeParameters::parseDouble(RuntimeParameters::formatDefault(0.5), "-key"), 0.5);
  validateEquals(RuntimeParameters::formatDefault(std::string("")), std::string("<none>"));
  validateEquals(RuntimeParameters::formatDefault(std::string("out/")), std::string("out/"));
  validateEquals(RuntimeParameters::formatTerminalTime(std::numeric_limits<double>::max()), std::string("Unbounded"));
  validateEquals(RuntimeParameters::formatTerminalTime(2.0), std::string("2"));
}

void
exahype2::runner::tests::RuntimeParametersTest::testBothSpellingsOfTheLoadBalancingSwitchAreConsumed() {
  exahype2::runner::RuntimeParameters longFormParameters;
  char                                executableName[] = "ExaHyPE";
  char                                longSpelling[]   = "--disableLoadBalancing";
  std::array<char*, 2>                longForm{executableName, longSpelling};
  int                                 numberOfLongFormArguments = static_cast<int>(longForm.size());
  validateEquals(longFormParameters.isLoadBalancingDisabled(), false);
  longFormParameters.applyCommandLineConfiguration(numberOfLongFormArguments, longForm.data());
  validateEquals(longFormParameters.isLoadBalancingDisabled(), true);
  validateEquals(numberOfLongFormArguments, 1);

  exahype2::runner::RuntimeParameters shortFormParameters;
  char                                shortSpelling[] = "-dlb";
  std::array<char*, 2>                shortForm{executableName, shortSpelling};
  int                                 numberOfShortFormArguments = static_cast<int>(shortForm.size());
  validateEquals(shortFormParameters.isLoadBalancingDisabled(), false);
  shortFormParameters.applyCommandLineConfiguration(numberOfShortFormArguments, shortForm.data());
  validateEquals(shortFormParameters.isLoadBalancingDisabled(), true);
  validateEquals(numberOfShortFormArguments, 1);
}

/**
 * A maximum terminal time from the command line or the environment lowers a larger minimum one.
 */
void
exahype2::runner::tests::RuntimeParametersTest::testTheMaximumTerminalTimeBoundsTheMinimumOne() {
  exahype2::runner::RuntimeParameters commandLineParameters;
  commandLineParameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");
  char                 executableName[] = "ExaHyPE";
  char                 option[]         = "-tmax";
  char                 bound[]          = "1.0";
  std::array<char*, 3> arguments{executableName, option, bound};
  int                  numberOfArguments = static_cast<int>(arguments.size());
  commandLineParameters.applyCommandLineConfiguration(numberOfArguments, arguments.data());
  commandLineParameters.boundTerminalTimes();
  validateNumericalEquals(commandLineParameters.getMinTerminalTime(), 1.0);
  validateNumericalEquals(commandLineParameters.getMaxTerminalTime(), 1.0);

  exahype2::runner::RuntimeParameters environmentParameters;
  environmentParameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");
  setenv("PEANO_MAX_TERMINAL_TIME", "0.5", 1); // NOLINT(concurrency-mt-unsafe)
  environmentParameters.boundTerminalTimes();
  validateNumericalEquals(environmentParameters.getMinTerminalTime(), 0.5);
  validateNumericalEquals(environmentParameters.getMaxTerminalTime(), 0.5);

  exahype2::runner::RuntimeParameters shorterRunParameters;
  shorterRunParameters.setGeneratedDefaults(0.1, 0.1, -1, -1, "");
  shorterRunParameters.boundTerminalTimes();
  unsetenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
  validateNumericalEquals(shorterRunParameters.getMinTerminalTime(), 0.1);
  validateNumericalEquals(shorterRunParameters.getMaxTerminalTime(), 0.1);
}

/**
 * Without the option the plot format is the default of the build, and both spellings select one.
 */
void
exahype2::runner::tests::RuntimeParametersTest::testThePlotFormatComesFromTheCommandLine() {
  const exahype2::runner::RuntimeParameters defaultParameters;
  validateEquals(
    static_cast<int>(defaultParameters.getPlotFormat()),
    static_cast<int>(tarch::plotter::getDefaultFormat())
  );

  exahype2::runner::RuntimeParameters longFormParameters;
  char                                executableName[] = "ExaHyPE";
  char                                longSpelling[]   = "--plotFormat";
  char                                vtu[]            = "vtu";
  std::array<char*, 3>                longForm{executableName, longSpelling, vtu};
  int                                 numberOfLongFormArguments = static_cast<int>(longForm.size());
  longFormParameters.applyCommandLineConfiguration(numberOfLongFormArguments, longForm.data());
  validateEquals(static_cast<int>(longFormParameters.getPlotFormat()), static_cast<int>(tarch::plotter::Format::VTU));
  validateEquals(numberOfLongFormArguments, 1);

  exahype2::runner::RuntimeParameters shortFormParameters;
  char                                shortSpelling[] = "-pf";
  char                                vtkhdf[]        = "vtkhdf";
  std::array<char*, 3>                shortForm{executableName, shortSpelling, vtkhdf};
  int                                 numberOfShortFormArguments = static_cast<int>(shortForm.size());
  shortFormParameters.applyCommandLineConfiguration(numberOfShortFormArguments, shortForm.data());
  validateEquals(
    static_cast<int>(shortFormParameters.getPlotFormat()),
    static_cast<int>(tarch::plotter::Format::VTKHDF)
  );
  validateEquals(numberOfShortFormArguments, 1);
}

void
exahype2::runner::tests::RuntimeParametersTest::testTheTableReadsTheCommandLineAndKeepsTheOtherArguments() {
  Storage   storage;
  Arguments arguments{
    "exe",
    "--speed",
    "2.5",
    "-lmax",
    "--count",
    "3",
    "--flags",
    "yes,0,TRUE",
    "--name",
    "x",
    "--offset",
    "1,2"
  };
  getTableOf(storage).load(nullptr, arguments.argc, arguments.argv.data());
  validateNumericalEquals(storage.speed, 2.5);
  validateEquals(storage.count, 3);
  validateEquals(storage.verbose, false);
  validateEquals(storage.name, std::string("x"));
  validateNumericalEquals(storage.offset(0), 1.0);
  validateNumericalEquals(storage.offset(1), 2.0);
  validateEquals(storage.flags(0), true);
  validateEquals(storage.flags(1), false);
  validateEquals(storage.flags(2), true);
  validateEquals(arguments.argc, 2);
  validateEquals(std::string(arguments.argv[1]), std::string("-lmax"));
  validateTrue(arguments.argv[2] == nullptr);
}

void
exahype2::runner::tests::RuntimeParametersTest::testTheTableReadsTheConfigurationFileBeforeTheCommandLine() {
  Storage           storage;
  const std::string configuration =
    writeConfiguration(R"({"speed": 4.0, "verbose": true, "offset": [3.0, 4.0], "count": 7})");
  const tarch::reader::JSONReader reader(configuration);
  std::filesystem::remove(configuration);
  Arguments arguments{"exe", "--count", "9"};
  getTableOf(storage).load(&reader, arguments.argc, arguments.argv.data());
  validateNumericalEquals(storage.speed, 4.0);
  validateEquals(storage.verbose, true);
  validateNumericalEquals(storage.offset(0), 3.0);
  validateNumericalEquals(storage.offset(1), 4.0);
  validateEquals(storage.count, 9);
  validateEquals(arguments.argc, 1);
}

void
exahype2::runner::tests::RuntimeParametersTest::testTheTableUsageNamesEveryEntryWithItsDefault() {
  Storage    storage;
  const auto usage = getTableOf(storage).getUsage();
  validateTrue(usage.find("--speed <double> (default: 1.5).") != std::string::npos);
  validateTrue(usage.find("--count <int> (default: 2).") != std::string::npos);
  validateTrue(usage.find("--verbose <bool> (default: false).") != std::string::npos);
  validateTrue(usage.find("--name <string> (default: <none>).") != std::string::npos);
  validateTrue(usage.find("--offset <double[2]> (default: 0,0).") != std::string::npos);
  validateTrue(usage.find("--flags <bool[3]> (default: false,false,false).") != std::string::npos);
  validateTrue(usage.find("Array values are comma-separated.") != std::string::npos);
  validateTrue(exahype2::runner::ParameterTable{}.getUsage().find("<none>") != std::string::npos);
}

/**
 * A configuration file sets the engine parameters it names, and an MPI timeout switches the request tracking on.
 */
void
exahype2::runner::tests::RuntimeParametersTest::testTheConfigurationFileSetsTheEngineParameters() {
  exahype2::runner::RuntimeParameters parameters;
  validateEquals(parameters.isMPIRequestTrackingEnabled(), false);
  parameters._configFilePath = writeConfiguration(
    R"({"timeoutWarning": 0.5, "verbose": true, "numberOfSnapshots": 3, "maxGridLevels": 4, "endTime": 2.0})"
  );
  parameters.readJSONConfiguration();
  std::filesystem::remove(parameters._configFilePath);
  validateNumericalEquals(parameters.getMPITimeoutWarning(), 0.5);
  validateEquals(parameters.isMPIRequestTrackingEnabled(), true);
  validateEquals(parameters.isVerbose(), true);
  validateEquals(parameters.getNumberOfSnapshots(), 3);
  validateEquals(parameters.getMaxGridLevels(), 4);
  validateNumericalEquals(parameters.getMinTerminalTime(), 2.0);
  validateTrue(parameters.getConfigReader() != nullptr);
}
