// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestCase.h"

#include <unistd.h>

namespace {
  std::filesystem::path determineWritableTestDirectory() {
    std::error_code             errorCode;
    const std::filesystem::path candidates[] = {
      std::filesystem::current_path(errorCode),
      std::filesystem::temp_directory_path(errorCode)
    };

    for (const auto& base : candidates) {
      if (base.empty()) {
        continue;
      }
      // The probe file name contains the process id, so concurrent ranks do not race on it.
      const std::filesystem::path probe = base / (".peano-write-probe-" + std::to_string(::getpid()));
      std::ofstream               stream(probe);
      if (stream.good()) {
        stream.close();
        std::filesystem::remove(probe, errorCode);
        return base;
      }
    }

    // Last resort: The current working directory, which the job owns.
    return std::filesystem::current_path(errorCode);
  }
} // namespace

/**
 * Returns the directory a test writes its files into, which is the same on every rank because the
 * parallel plotters open one file collectively. The function probes the working directory first
 * and falls back to the system temporary directory.
 */
std::filesystem::path
tarch::tests::getWritableTestDirectory() {
  static const std::filesystem::path directory = determineWritableTestDirectory();
  return directory;
}

/**
 * Returns MaxUnitsInTheLastPlace units in the last place of the largest magnitude of reference, the
 * tolerance of validateNumericalBuffersEquals.
 */
double
tarch::tests::getToleranceInUnitsInTheLastPlace(const std::vector<double>& reference) {
  double largestMagnitude = 0.0;
  for (const double value : reference) {
    largestMagnitude = std::max(largestMagnitude, std::abs(value));
  }
  const double unitInTheLastPlace =
    std::nextafter(largestMagnitude, std::numeric_limits<double>::infinity()) - largestMagnitude;
  // A compiler that flushes subnormal results to zero turns a subnormal tolerance into an exact
  // comparison, so the smallest normal double is the floor.
  return MaxUnitsInTheLastPlace * std::max(unitInTheLastPlace, std::numeric_limits<double>::min());
}

tarch::tests::TestCase::TestCase(const std::string& testCaseName) :
  _testCaseName(testCaseName) {}

tarch::tests::TestCase::~TestCase() = default;

int
tarch::tests::TestCase::getNumberOfErrors() const {
  return _errors;
}

std::string
tarch::tests::TestCase::getTestCaseName() const {
  return _testCaseName;
}
