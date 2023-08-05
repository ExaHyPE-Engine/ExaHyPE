#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator
namespace tarch::accelerator::omp {
  /**
   * Queries device information for OpenMP GPU accelerator  backend.
   *
   * It implements the device infromation queries described in `tarch::accelerator::queryDeviceInformation`
   * Since OpenMP hides any information about the GPUs other than the count to the users, a matrix that is
   * filled will placeholder "X" values instead of the information is returned.
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Prints device information for OpenMP GPU accelerator  backend.
   *
   * It implements the device infromation printing described in `tarch::accelerator::printDeviceInformation`
   *
   * @see tarch::accelerator::printDeviceInformation
   */
  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Prints device information for OpenMP GPU accelerator  backend.
   *
   * It implements the kernel launch test described in `tarch::accelerator::testKernelLaunch`
   *
   *
   * @see tarch::accelerator::testKernelLaunch
   */
  std::vector<bool> testKernelLaunch();

  /**
   * Runs benchmarks for the OpenMP GPU accelerator backend.
   *
   * It implements the memory bandwidth benchmarks described in `tarch::accelerator::runBenchmarks`
   *
   * @see tarch::accelerator::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::omp
