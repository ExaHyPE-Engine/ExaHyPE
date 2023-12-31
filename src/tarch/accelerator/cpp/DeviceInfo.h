// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::cpp {
  /**
   * @brief Queries device information for the C++ accelerator implementation.
   *
   * @return A single mock-up accelerator with no information available
   * @param propertyMatrix The matrix to store the dummy device properties.
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Print device information based on the provided property matrix.
   *
   * The C++ accelerator implementation does not have the means to query the number of device properties,
   * so the property matrix is not used in this function.
   *
   * @param propertyMatrix The property matrix (unused).
   */
  void printDeviceInformation([[maybe_unused]] std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Test the kernel launch capability of the system.
   *
   * This function tests the kernel launch capability of the system.
   * The runtime check is not necessary as `_NVHPC_STDPAR_GPU` indicates we have
   * C++ GPU offloading available.
   *
   * @return A vector indicating the kernel launch capability.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Runs benchmarks with the C++ accelerator implementation.
   *
   * C++ does not expose devices, so the benchmarks are executed in a unified manner
   * and the results contain only one GPU.
   *
   * Although not explicitly done, the first touch in the GPU kernel also involves copying the data over to the GPU.
   * And the second touch works on the data that already resides in the GPU, this way we can reproduce the streaming benchmark.
   *
   * @return A vector containing the benchmark results.
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::cpp
