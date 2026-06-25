// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestCase.h"

#include <fstream>
#include <string>
#include <unistd.h>

namespace {
  std::filesystem::path determineWritableTestDirectory() {
    std::error_code             ec;
    const std::filesystem::path candidates[]
      = {std::filesystem::current_path(ec), std::filesystem::temp_directory_path(ec)};

    for (const auto& base : candidates) {
      if (base.empty()) {
        continue;
      }
      // Probe with a per-process file name so concurrent ranks do not race on
      // the probe, but hand back the shared base directory itself.
      const std::filesystem::path probe = base / (".peano-write-probe-" + std::to_string(::getpid()));
      std::ofstream               stream(probe);
      if (stream.good()) {
        stream.close();
        std::filesystem::remove(probe, ec);
        return base;
      }
    }

    // Last resort: the current working directory, which the job owns.
    return std::filesystem::current_path(ec);
  }
} // namespace

std::filesystem::path tarch::tests::getWritableTestDirectory() {
  // Magic-static initialisation: thread-safe and evaluated only once.
  static const std::filesystem::path directory = determineWritableTestDirectory();
  return directory;
}

tarch::tests::TestCase::TestCase(const std::string& testCaseName):
  _testCaseName(testCaseName),
  _errors(0),
  _error(false) {}

tarch::tests::TestCase::~TestCase() {}

int tarch::tests::TestCase::getNumberOfErrors() const { return _errors; }

std::string tarch::tests::TestCase::getTestCaseName() const { return _testCaseName; }
