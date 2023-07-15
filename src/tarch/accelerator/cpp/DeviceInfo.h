#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::cpp {
  static tarch::logging::Log _log("tarch::accelerator::cpp");

  /**
   * Queries device information for ISO C++ GPU accelerator  backend.
   *
   * It implements the device infromation queries described in `tarch::accelerator::queryDeviceInformation`
   * Since ISO C++ hides any information about the GPUs to the users, an empty matrix will be returned
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Prints device information for ISO C++ GPU accelerator  backend.
   *
   * It implements the device infromation printing described in `tarch::accelerator::printDeviceInformation`
   *
   * Individual backend's have the choice to not print a table if the information is too little.
   * This is the case in ISO C++ backend.
   *
   * @see tarch::accelerator::printDeviceInformation
   */
  void printDeviceInformation([[maybe_unused]] std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Runs benchmarks for the ISO C++ GPU accelerator backend.
   *
   * It implements the benchmarks described in `tarch::accelerator::testKernelLaunch`
   *
   * @see tarch::accelerator::testKernelLaunch
   */
  std::vector<bool> testKernelLaunch();

  /**
   * Runs benchmarks for the ISO C++ GPU accelerator backend.
   *
   * It implements  the benchmarks described in `tarch::accelerator::runBenchmarks`
   *
   * @see tarch::accelerator::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::cpp
