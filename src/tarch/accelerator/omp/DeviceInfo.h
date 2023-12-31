// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::omp {
  /**
   * @brief Queries device information for the OpenMP target accelerator implementation.
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Prints device information for the OpenMP target accelerator implementation.
   *
   * @see tarch::accelerator::printDeviceInformation
   */
  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Test the kernel launch capability of the system.
   *
   * @see tarch::accelerator::testKernelLaunch
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Runs benchmarks with the the OpenMP target accelerator implementation.
   *
   * @see tarch::accelerator::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::omp
