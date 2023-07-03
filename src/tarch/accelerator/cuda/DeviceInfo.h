#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator
{
  struct BenchmarkResultInNanoSeconds;
}

namespace tarch::accelerator::cuda
{
  static tarch::logging::Log _log("tarch::accelerator::cuda");

  /**
   * @brief Query device information using CUDA API.
   *
   * This function queries device information using the CUDA API. 
   * It retrieves properties of each device, such as the device name, 
   * global memory, free memory, clock frequency, and compute units. 
   * The information is stored in the provided property matrix.
   *
   * @param propertyMatrix A reference to a matrix that will store the device properties.
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Print device information when there is access to the CUDA API.
   *
   * This function prints the device information using the CUDA API. 
   * It takes a property matrix containing the device properties and outputs the information to the log.
   *
   * @param propertyMatrix A reference to a matrix containing the device properties.
   */
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Test kernel launch capability.
   *
   * This function tests the capability of kernel launch based on the available GPU offloading backend.
   * The kernels reside in a separate CUDA file. The function will compiled only if we use CUDA for GPU 
   * offloading, unlike the query/information functions where only the CUDA API is called to retrieve information,
   * here CUDA kernels will be launched. And therefore the compiling is hidden behind pre processor flags.
   *
   * @return A vector of bool values indicating the kernel launch capability for each device.
   */
  std::vector<bool> testKernelLaunch();

/**
 * @brief Run benchmarks with CUDA backend by launch kernels to perform the streaming benchmarks.
 *
 * This function runs benchmarks to measure the performance of various GPU operations using the CUDA backend. 
 * It queries the number of devices using the CUDA backend and performs benchmarks for each device. 
 * The benchmarks include memory allocation, data transfers between CPU and GPU, and kernel execution. 
 * The results are collected in a vector of `tarch::accelerator::BenchmarkResultInNanoSeconds` structures.
 * The function is compiled only if GPUOffloadingCUDA is defined. see `testKernelLaunch` too.
 * We allocate roughly 1GB of data on the CPU then transfer it to the GPU, and add half of this
 * data to the other half and transfer it back. Every step is then measured and saved in the BenchmarkResultsInNanoSeconds
 * struct to be printed as a table.
 *
 * @return A vector of `tarch::accelerator::BenchmarkResultInNanoSeconds` structures containing benchmark results for each device.
 * 
 * @see tarch::accelerator::omp::runBenchmarks
 * @see tarch::accelerator::cpp::runBenchmarks
 * @see tarch::accelerator::sycl::runBenchmarks
 */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::cuda
