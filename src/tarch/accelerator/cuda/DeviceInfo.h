// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "config.h"
#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::cuda {
  /**
   * @brief Queries device information for the CUDA accelerator implementation.
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Print device information when there is access to the CUDA API.
   *
   * @param propertyMatrix A reference to a matrix containing the device properties.
   */
  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Test kernel launch capability.
   *
   * This function tests the capability of launching CUDA kernels.
   * The kernels reside in a separate .cu file.
   *
   * @return A vector of bool values indicating the kernel launch capability for each device.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Runs benchmarks with the CUDA accelerator implementation.
   *
   * @return A vector containing the benchmark results.
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::cuda
