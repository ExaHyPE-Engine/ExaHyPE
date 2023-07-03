#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator
{
  struct BenchmarkResultInNanoSeconds;
}
namespace tarch::accelerator::omp
{
  static tarch::logging::Log _log("tarch::accelerator::omp");

  /**
   * @brief Query device information and update the property matrix.
   *
   * Only information OpenMP GPU Offloading can return as information is the number of GPUs
   *
   * @param[in,out] propertyMatrix A matrix of property values to be updated with the device information.
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Prints the whatever little information the OpenMP backend provides
   *
   * @param[in,out] propertyMatrix A matrix of property values
   */
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Run offloading tests with OpenMP.
   *
   * This function runs benchmarks to check whether offloading can be performed
   *
   * @return A vector of boolean to indicate successful offloading for the device.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Run benchmarks with OpenMP backend.
   *
   * This function runs benchmarks using the OpenMP backend and measures the performance of various operations.
   *
   * An attempt to dynamically allocate more data until there is no space in GPU fails as the attempt to allocate
   * results with a runtime error that also has a termination SIGNAL, which although being catchable, trying to
   * avoid the termination is undefined behavior, and often the compilers just terminate. Therefore dynamic allocation
   * tests is not possible. We allocate roughly 1GB of data on the CPU then transfer it to the GPU, and add half of this
   * data to the other half and transfer it back. Every step is then measured and saved in the BenchmarkResultsInNanoSeconds
   * struct to be printed as a table.
   *
   * @return A vector of BenchmarkResultInNanoSeconds objects containing the benchmark results for each device.
   * 
   * @see tarch::accelerator::cpp::runBenchmarks
   * @see tarch::accelerator::cuda::runBenchmarks
   * @see tarch::accelerator::sycl::runBenchmarks
   */ 
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::omp
