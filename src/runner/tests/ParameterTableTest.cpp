// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParameterTableTest.h"

#include "runner/ParameterTable.h"
#include "tarch/Termination.h"
#include "tarch/la/Vector.h"
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

  runner::ParameterTable tableOf(Storage& storage) {
    runner::ParameterTable table;
    table.add("speed", &storage.speed);
    table.add("count", &storage.count);
    table.add("verbose", &storage.verbose);
    table.add("name", &storage.name);
    table.addArray("offset", storage.offset.data(), 2);
    table.addArray("flags", storage.flags.data(), 3);
    return table;
  }

  std::string writeConfiguration(const std::string& content) {
    // The file name carries a per-run identifier, so two processes that write into the test
    // directory at the same time do not overwrite each other's fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::tests::getWritableTestDirectory()
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

runner::tests::ParameterTableTest::ParameterTableTest() :
  TestCase("runner::tests::ParameterTableTest") {}

void runner::tests::ParameterTableTest::run() {
  testMethod(testCommandLineSetsEntriesAndKeepsTheOtherArguments);
  testMethod(testConfigurationFileSetsEntriesBeforeTheCommandLine);
  testMethod(testUsageNamesEveryEntryWithItsDefault);
}

void runner::tests::ParameterTableTest::testCommandLineSetsEntriesAndKeepsTheOtherArguments() {
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

  tableOf(storage).load(nullptr, arguments.argc, arguments.argv.data());

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

void runner::tests::ParameterTableTest::testConfigurationFileSetsEntriesBeforeTheCommandLine() {
  Storage                         storage;
  const tarch::reader::JSONReader reader(
    writeConfiguration(R"({"speed": 4.0, "verbose": true, "offset": [3.0, 4.0], "count": 7})")
  );
  Arguments arguments{"exe", "--count", "9"};

  tableOf(storage).load(&reader, arguments.argc, arguments.argv.data());

  validateNumericalEquals(storage.speed, 4.0);
  validateEquals(storage.verbose, true);
  validateNumericalEquals(storage.offset(0), 3.0);
  validateNumericalEquals(storage.offset(1), 4.0);
  validateEquals(storage.count, 9);
  validateEquals(arguments.argc, 1);
}

void runner::tests::ParameterTableTest::testUsageNamesEveryEntryWithItsDefault() {
  Storage    storage;
  const auto usage = tableOf(storage).usage();
  validateTrue(usage.find("--speed <double> (default: 1.5).") != std::string::npos);
  validateTrue(usage.find("--count <int> (default: 2).") != std::string::npos);
  validateTrue(usage.find("--verbose <bool> (default: false).") != std::string::npos);
  validateTrue(usage.find("--name <string> (default: <none>).") != std::string::npos);
  validateTrue(usage.find("--offset <double[2]> (default: 0,0).") != std::string::npos);
  validateTrue(usage.find("--flags <bool[3]> (default: false,false,false).") != std::string::npos);
  validateTrue(usage.find("Array values are comma-separated.") != std::string::npos);
  validateTrue(runner::ParameterTable{}.usage().find("<none>") != std::string::npos);
}
