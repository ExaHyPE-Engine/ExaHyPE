#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::sycl {
  static tarch::logging::Log _log("tarch::accelerator::sycl");

  /**
   * Queries device information for Sycl GPU accelerator  backend.
   *
   * It implements the device infromation queries described in `tarch::accelerator::queryDeviceInformation`
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Prints device information for Sycl GPU accelerator  backend.
   *
   * It implements the device infromation printing described in `tarch::accelerator::printDeviceInformation`
   *
   * Individual backend's have the choice to not print a table if the information is too little.
   *
   * @see tarch::accelerator::printDeviceInformation
   */
  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Runs benchmarks for the Sycl GPU accelerator backend.
   *
   * It implements the kernel launch test described in `tarch::accelerator::testKernelLaunch`
   *
   * @see tarch::accelerator::testKernelLaunch
   */
  std::vector<bool> testKernelLaunch();

  /**
   * Runs benchmarks for the Sycl GPU accelerator backend.
   *
   * It implements the memory bandwidth benchmarks described in `tarch::accelerator::runBenchmarks`
   *
   * @see tarch::accelerator::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::sycl
