#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator
{
  struct BenchmarkResultInNanoSeconds;
}

namespace tarch::accelerator::sycl
{
  static tarch::logging::Log _log("tarch::accelerator::sycl");

  /**
   * @brief Query device information and update the property matrix.
   *
   * This function queries the device information using SYCL and updates the `propertyMatrix` with
   * the retrieved information. The `propertyMatrix` is expected to be a 2D vector of strings,
   * where each row represents a property and each column represents a device.
   *
   * @param[in,out] propertyMatrix A matrix of property values to be updated with the device information.
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Print device information.
   *
   *
   * @param propertyMatrix A matrix containing the device properties.
   */
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Test the kernel launch with SYCL backend.
   *
   * This function tests the kernel launch using the SYCL backend. It runs a simple kernel on each
   * GPU device and checks if the kernel was successfully executed by verifying the value of a boolean
   * variable. The function returns a vector of bool values indicating the success of the kernel launch
   * on each device.
   *
   * @return A vector of bool values indicating the success of the kernel launch on each device.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Return empty benchmark results as SYCL benchmarks are being implemented
   * 
   * @return A vector of obtained benchmark results
   * 
   * @see tarch::accelerator::cpp::runBenchmarks
   * @see tarch::accelerator::cuda::runBenchmarks
   * @see tarch::accelerator::omp::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::sycl
