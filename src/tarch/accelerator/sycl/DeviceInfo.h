// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::sycl {
  /**
   * @brief Queries device information for SYCL accelerator implementation.
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Prints device information for the SYCL accelerator implementation.
   *
   * @param[in,out] propertyMatrix A matrix of property values to be updated with the device information.
   */
  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Test the kernel launch capability of the system.
   *
   * @return A vector of bool values indicating the success of the kernel launch on each device.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Runs benchmarks with the the SYCL accelerator implementation.
   *
   * @see tarch::accelerator::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::sycl
