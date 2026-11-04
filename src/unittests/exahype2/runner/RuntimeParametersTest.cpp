// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/runner/RuntimeParameters.h"

#include "peano4/CellConnectivity.h"
#include "tarch/Termination.h"
#include "tarch/la/Vector.h"
#include "tarch/plotter/Format.h"
#include "tarch/reader/JSONReader.h"
#include "tarch/testing/UnitTest.h"

namespace {
  using RuntimeParameters = exahype2::runner::RuntimeParameters;

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
   * Writes content to a new file whose name contains a per-run identifier. Two processes never overwrite
   * each other's file. The caller deletes the file.
   */
  std::string writeConfiguration(const std::string& content) {
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int>     counter = 0;
    const std::filesystem::path path    = tarch::testing::getWritableTestDirectory()
      / std::filesystem::path("parameter-table-" + testRunIdentifier + "-" + std::to_string(counter++) + ".json");
    std::ofstream output(path);
    if (not output.is_open()) {
      tarch::terminateWithError("The test cannot create the temporary file '" + path.string() + "'.");
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

  /**
   * Deletes the configuration file and configures the runtime parameters with an empty configuration file path. Later
   * configurations skip the deleted file.
   */
  void deleteConfigurationFile(const std::string& configuration) {
    std::filesystem::remove(configuration);
    Arguments withoutConfigurationFile{"ExaHyPE", "--configFile", ""};
    RuntimeParameters::getInstance().configure(withoutConfigurationFile.argc, withoutConfigurationFile.argv.data());
  }
} // namespace

UNIT_TEST("Numbers parse from the whole argument, signs and exponents included") {
  GIVEN("Two storages and command lines with the integers 42 and -7 and the floating point numbers 0.25 and -1.5e2") {
    Storage   positive;
    Storage   negative;
    Arguments positiveArguments{"exe", "--count", "42", "--speed", "0.25"};
    Arguments negativeArguments{"exe", "--count", "-7", "--speed", "-1.5e2"};

    WHEN("The table over each storage loads its command line") {
      getTableOf(positive).load(nullptr, positiveArguments.argc, positiveArguments.argv.data());
      getTableOf(negative).load(nullptr, negativeArguments.argc, negativeArguments.argv.data());

      THEN("The parsed values are 42, -7, 0.25 and -150") {
        VALIDATE_EQUALS(positive.count, 42);
        VALIDATE_EQUALS(negative.count, -7);
        VALIDATE_NUMERICAL_EQUALS(positive.speed, 0.25);
        VALIDATE_NUMERICAL_EQUALS(negative.speed, -150.0);
      }
    }
  }
}

UNIT_TEST("The output path always ends in a separator") {
  GIVEN("The runtime parameters") {
    auto& parameters = RuntimeParameters::getInstance();

    WHEN("The command line sets a relative path, the same path with a separator, an absolute path and the empty path") {
      const auto getOutputPathOf = [&parameters](const std::string& path) {
        Arguments arguments{"ExaHyPE", "-o", path};
        parameters.configure(arguments.argc, arguments.argv.data());
        return parameters.getOutputPath();
      };
      const std::string relativePath      = getOutputPathOf("solutions");
      const std::string pathWithSeparator = getOutputPathOf("solutions/");
      const std::string absolutePath      = getOutputPathOf("/tmp/a/b");
      const std::string emptyPath         = getOutputPathOf("");
      const std::string defaultPath       = getOutputPathOf("output");

      THEN("Every path ends in one separator and the empty path names the working directory") {
        VALIDATE_EQUALS(relativePath, std::string("solutions/"));
        VALIDATE_EQUALS(pathWithSeparator, std::string("solutions/"));
        VALIDATE_EQUALS(absolutePath, std::string("/tmp/a/b/"));
        VALIDATE_EQUALS(emptyPath, std::string("./"));
        VALIDATE_EQUALS(defaultPath, std::string("output/"));
      }
    }
  }
}

UNIT_TEST("The admissible patch sizes divide the longest domain edge by the subdivision factor once per level") {
  GIVEN("The runtime parameters and a domain whose edges are the square of the subdivision factor") {
    auto&                                       parameters        = RuntimeParameters::getInstance();
    constexpr double                            SubdivisionFactor = peano4::SubdivisionFactor;
    const tarch::la::Vector<DIMENSIONS, double> domainSize(SubdivisionFactor * SubdivisionFactor);

    WHEN("The command line sets the coarsest grid level one and the finest grid level two") {
      Arguments levels{"ExaHyPE", "-lmin", "1", "-lmax", "2"};
      parameters.configure(levels.argc, levels.argv.data());
      const double maxPatchSize = parameters.getMaxAdmissiblePatchSize(domainSize);
      const double minPatchSize = parameters.getMinAdmissiblePatchSize(domainSize);
      Arguments    defaultLevels{"ExaHyPE", "-l", "3"};
      parameters.configure(defaultLevels.argc, defaultLevels.argv.data());

      THEN("The largest patch is 1.1 times the subdivision factor and the smallest patch is 1.1") {
        VALIDATE_NUMERICAL_EQUALS(maxPatchSize, 1.1 * SubdivisionFactor);
        VALIDATE_NUMERICAL_EQUALS(minPatchSize, 1.1);
      }
    }
  }
}

UNIT_TEST("A terminal time reads Unbounded for the largest double and its number otherwise") {
  GIVEN("The largest double and the terminal time two") {
    constexpr double LargestDouble = std::numeric_limits<double>::max();
    constexpr double TerminalTime  = 2.0;

    WHEN("The function formatTerminalTime() formats both values") {
      const std::string unboundedTerminalTime = RuntimeParameters::formatTerminalTime(LargestDouble);
      const std::string formattedTerminalTime = RuntimeParameters::formatTerminalTime(TerminalTime);

      THEN("The largest double reads Unbounded and two reads 2") {
        VALIDATE_EQUALS(unboundedTerminalTime, std::string("Unbounded"));
        VALIDATE_EQUALS(formattedTerminalTime, std::string("2"));
      }
    }
  }
}

UNIT_TEST("Both spellings of the load balancing switch disable the load balancing and leave the command line") {
  GIVEN("The runtime parameters and a configuration file that enables the load balancing") {
    auto&             parameters    = RuntimeParameters::getInstance();
    const std::string configuration = writeConfiguration(R"({"disableLoadBalancing": false})");

    WHEN("The file configures the parameters and then the file together with the long switch") {
      Arguments fileOnly{"ExaHyPE", "--configFile", configuration};
      parameters.configure(fileOnly.argc, fileOnly.argv.data());
      const bool disabledBeforeTheSwitch = parameters.isLoadBalancingDisabled();
      Arguments  withTheSwitch{"ExaHyPE", "--configFile", configuration, "--disableLoadBalancing"};
      parameters.configure(withTheSwitch.argc, withTheSwitch.argv.data());
      const bool disabledByTheSwitch = parameters.isLoadBalancingDisabled();
      deleteConfigurationFile(configuration);

      THEN("The file enables the load balancing and the switch disables it and leaves the command line") {
        VALIDATE_EQUALS(disabledBeforeTheSwitch, false);
        VALIDATE_EQUALS(disabledByTheSwitch, true);
        VALIDATE_EQUALS(withTheSwitch.argc, 1);
      }
    }

    WHEN("The file configures the parameters and then the file together with the short switch") {
      Arguments fileOnly{"ExaHyPE", "--configFile", configuration};
      parameters.configure(fileOnly.argc, fileOnly.argv.data());
      const bool disabledBeforeTheSwitch = parameters.isLoadBalancingDisabled();
      Arguments  withTheSwitch{"ExaHyPE", "--configFile", configuration, "-dlb"};
      parameters.configure(withTheSwitch.argc, withTheSwitch.argv.data());
      const bool disabledByTheSwitch = parameters.isLoadBalancingDisabled();
      deleteConfigurationFile(configuration);

      THEN("The file enables the load balancing and the switch disables it and leaves the command line") {
        VALIDATE_EQUALS(disabledBeforeTheSwitch, false);
        VALIDATE_EQUALS(disabledByTheSwitch, true);
        VALIDATE_EQUALS(withTheSwitch.argc, 1);
      }
    }
  }
}

UNIT_TEST("A maximum terminal time from the command line or the environment lowers a larger minimum one") {
  GIVEN("The runtime parameters with the generated minimum and maximum terminal time ten") {
    auto& parameters = RuntimeParameters::getInstance();
    parameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");

    WHEN("The command line bounds the maximum terminal time by one") {
      Arguments arguments{"ExaHyPE", "-tmax", "1.0"};
      parameters.configure(arguments.argc, arguments.argv.data());
      const double minTerminalTime = parameters.getMinTerminalTime();
      const double maxTerminalTime = parameters.getMaxTerminalTime();

      THEN("Both terminal times are one") {
        VALIDATE_NUMERICAL_EQUALS(minTerminalTime, 1.0);
        VALIDATE_NUMERICAL_EQUALS(maxTerminalTime, 1.0);
      }
    }

    WHEN("The environment bounds the maximum terminal time by one half") {
      setenv("PEANO_MAX_TERMINAL_TIME", "0.5", 1); // NOLINT(concurrency-mt-unsafe)
      Arguments arguments{"ExaHyPE"};
      parameters.configure(arguments.argc, arguments.argv.data());
      unsetenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
      const double minTerminalTime = parameters.getMinTerminalTime();
      const double maxTerminalTime = parameters.getMaxTerminalTime();

      THEN("Both terminal times are one half") {
        VALIDATE_NUMERICAL_EQUALS(minTerminalTime, 0.5);
        VALIDATE_NUMERICAL_EQUALS(maxTerminalTime, 0.5);
      }
    }
  }

  GIVEN("The runtime parameters with the generated minimum and maximum terminal time one tenth") {
    auto& parameters = RuntimeParameters::getInstance();
    parameters.setGeneratedDefaults(0.1, 0.1, -1, -1, "");

    WHEN("The environment bounds the maximum terminal time by one half") {
      setenv("PEANO_MAX_TERMINAL_TIME", "0.5", 1); // NOLINT(concurrency-mt-unsafe)
      Arguments arguments{"ExaHyPE"};
      parameters.configure(arguments.argc, arguments.argv.data());
      unsetenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
      const double minTerminalTime = parameters.getMinTerminalTime();
      const double maxTerminalTime = parameters.getMaxTerminalTime();

      THEN("Both terminal times stay one tenth") {
        VALIDATE_NUMERICAL_EQUALS(minTerminalTime, 0.1);
        VALIDATE_NUMERICAL_EQUALS(maxTerminalTime, 0.1);
      }
    }
  }
}

UNIT_TEST("A step budget leaves the solvers without a terminal time") {
  GIVEN("The runtime parameters with the generated minimum and maximum terminal time ten") {
    auto& parameters = RuntimeParameters::getInstance();
    parameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");

    WHEN("The command line sets the step budget zero") {
      Arguments arguments{"ExaHyPE", "-nt", "0"};
      parameters.configure(arguments.argc, arguments.argv.data());
      const double solverTerminalTime = parameters.getSolverTerminalTime();

      THEN("The solver terminal time is the minimum terminal time ten") {
        VALIDATE_NUMERICAL_EQUALS(solverTerminalTime, 10.0);
      }
    }

    WHEN("The command line sets a budget of five time steps") {
      Arguments budget{"ExaHyPE", "-nt", "5"};
      parameters.configure(budget.argc, budget.argv.data());
      const double solverTerminalTime = parameters.getSolverTerminalTime();
      Arguments    withoutBudget{"ExaHyPE", "-nt", "0"};
      parameters.configure(withoutBudget.argc, withoutBudget.argv.data());

      THEN("The solver terminal time is the largest double") {
        VALIDATE_EQUALS(solverTerminalTime, std::numeric_limits<double>::max());
      }
    }
  }
}

UNIT_TEST("The plot format is the default of the build until the command line selects one in either spelling") {
  GIVEN("The runtime parameters") {
    auto& parameters = RuntimeParameters::getInstance();

    WHEN("The parameters report their plot format and then the long option selects VTU and the short one VTKHDF") {
      const tarch::plotter::Format defaultFormat = parameters.getPlotFormat();
      Arguments                    longOption{"ExaHyPE", "--plotFormat", "vtu"};
      parameters.configure(longOption.argc, longOption.argv.data());
      const tarch::plotter::Format longOptionFormat = parameters.getPlotFormat();
      Arguments                    shortOption{"ExaHyPE", "-pf", "vtkhdf"};
      parameters.configure(shortOption.argc, shortOption.argv.data());
      const tarch::plotter::Format shortOptionFormat = parameters.getPlotFormat();

      THEN("The format follows the build default with VTU and then VTKHDF and both options leave the command line") {
        VALIDATE_EQUALS(static_cast<int>(defaultFormat), static_cast<int>(tarch::plotter::getDefaultFormat()));
        VALIDATE_EQUALS(static_cast<int>(longOptionFormat), static_cast<int>(tarch::plotter::Format::VTU));
        VALIDATE_EQUALS(longOption.argc, 1);
        VALIDATE_EQUALS(static_cast<int>(shortOptionFormat), static_cast<int>(tarch::plotter::Format::VTKHDF));
        VALIDATE_EQUALS(shortOption.argc, 1);
      }
    }
  }
}

UNIT_TEST("The table reads the command line and keeps the other arguments") {
  GIVEN("A storage and a command line that sets every entry but the verbose flag, with the option -lmax in between") {
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

    WHEN("The table over the storage loads the command line without a configuration file") {
      getTableOf(storage).load(nullptr, arguments.argc, arguments.argv.data());

      THEN("Every entry stores its value from the command line, the verbose flag its default and only -lmax remains") {
        VALIDATE_NUMERICAL_EQUALS(storage.speed, 2.5);
        VALIDATE_EQUALS(storage.count, 3);
        VALIDATE_EQUALS(storage.verbose, false);
        VALIDATE_EQUALS(storage.name, std::string("x"));
        VALIDATE_NUMERICAL_EQUALS(storage.offset(0), 1.0);
        VALIDATE_NUMERICAL_EQUALS(storage.offset(1), 2.0);
        VALIDATE_EQUALS(storage.flags(0), true);
        VALIDATE_EQUALS(storage.flags(1), false);
        VALIDATE_EQUALS(storage.flags(2), true);
        VALIDATE_EQUALS(arguments.argc, 2);
        VALIDATE_EQUALS(std::string(arguments.argv[1]), std::string("-lmax"));
        VALIDATE(arguments.argv[2] == nullptr);
      }
    }
  }
}

UNIT_TEST("The table reads the configuration file before the command line") {
  GIVEN("A storage, the reader of a configuration file with four entries and a command line that sets the count") {
    Storage           storage;
    const std::string configuration =
      writeConfiguration(R"({"speed": 4.0, "verbose": true, "offset": [3.0, 4.0], "count": 7})");
    const tarch::reader::JSONReader reader(configuration);
    std::filesystem::remove(configuration);
    Arguments arguments{"exe", "--count", "9"};

    WHEN("The table over the storage loads the configuration file and the command line") {
      getTableOf(storage).load(&reader, arguments.argc, arguments.argv.data());

      THEN("The file sets its entries, the command line overrides the count and argv keeps the name only") {
        VALIDATE_NUMERICAL_EQUALS(storage.speed, 4.0);
        VALIDATE_EQUALS(storage.verbose, true);
        VALIDATE_NUMERICAL_EQUALS(storage.offset(0), 3.0);
        VALIDATE_NUMERICAL_EQUALS(storage.offset(1), 4.0);
        VALIDATE_EQUALS(storage.count, 9);
        VALIDATE_EQUALS(arguments.argc, 1);
      }
    }
  }
}

UNIT_TEST("The table usage lists every entry with its default") {
  GIVEN("A storage") {
    Storage storage;

    WHEN("The table over the storage and an empty table write their usage") {
      const std::string usage      = getTableOf(storage).getUsage();
      const std::string emptyUsage = exahype2::runner::ParameterTable{}.getUsage();

      THEN("The usage lists every entry with type and default and explains arrays and the empty usage reads <none>") {
        VALIDATE(usage.find("--speed <double> (default: 1.5).") != std::string::npos);
        VALIDATE(usage.find("--count <int> (default: 2).") != std::string::npos);
        VALIDATE(usage.find("--verbose <bool> (default: false).") != std::string::npos);
        VALIDATE(usage.find("--name <string> (default: <none>).") != std::string::npos);
        VALIDATE(usage.find("--offset <double[2]> (default: 0,0).") != std::string::npos);
        VALIDATE(usage.find("--flags <bool[3]> (default: false,false,false).") != std::string::npos);
        VALIDATE(usage.find("Array values are comma-separated.") != std::string::npos);
        VALIDATE(emptyUsage.find("<none>") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A configuration file sets the engine parameters and its MPI timeout switches the request tracking on") {
  GIVEN("The runtime parameters and a file with the timeout warning, verbose output, snapshots, levels and end time") {
    auto&             parameters    = RuntimeParameters::getInstance();
    const std::string configuration = writeConfiguration(
      R"({"timeoutWarning": 0.5, "verbose": true, "numberOfSnapshots": 3, "maxGridLevels": 4, "endTime": 2.0})"
    );

    WHEN("The file configures the parameters") {
      const bool trackingBeforeTheFile = parameters.isMPIRequestTrackingEnabled();
      Arguments  arguments{"ExaHyPE", "--configFile", configuration};
      parameters.configure(arguments.argc, arguments.argv.data());
      const double timeoutWarning  = parameters.getMPITimeoutWarning();
      const bool   tracking        = parameters.isMPIRequestTrackingEnabled();
      const bool   verbose         = parameters.isVerbose();
      const int    snapshots       = parameters.getNumberOfSnapshots();
      const int    maxGridLevels   = parameters.getMaxGridLevels();
      const double minTerminalTime = parameters.getMinTerminalTime();
      const bool   hasReader       = parameters.getConfigReader() != nullptr;
      deleteConfigurationFile(configuration);

      THEN("The parameters store the values of the file and the timeout switches the request tracking on") {
        VALIDATE_EQUALS(trackingBeforeTheFile, false);
        VALIDATE_NUMERICAL_EQUALS(timeoutWarning, 0.5);
        VALIDATE_EQUALS(tracking, true);
        VALIDATE_EQUALS(verbose, true);
        VALIDATE_EQUALS(snapshots, 3);
        VALIDATE_EQUALS(maxGridLevels, 4);
        VALIDATE_NUMERICAL_EQUALS(minTerminalTime, 2.0);
        VALIDATE(hasReader);
      }
    }
  }
}
