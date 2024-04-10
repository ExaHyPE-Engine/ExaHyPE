#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  struct BenchmarkResultInNanoSeconds;
} // namespace tarch::accelerator

namespace tarch::accelerator::cpp {
  /**
   * Queries device information for C++ GPU accelerator  backend.
   *
   * It implements the device infromation queries described in `tarch::accelerator::queryDeviceInformation`
   * Since C++ hides any information about the GPUs to the users, an empty matrix will be returned.
   *
   * @see tarch::accelerator::queryDeviceInformation
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * @brief Returns a single mock-up GPU which no infformation available
   *
   * Returns a single mock-up GPU which no infformation available because C++ backend
   * hides any information off the accelerators from the user, yet one can run benchmarks with it 
   * where the workload will be automatically distributed between GPUs therewore having a line in the table
   * is still necessary for the benchmarks.
   *
   * @param propertyMatrix The matrix to store the dummy device properties of the unified GPU to be used by the benchmarks.
   */
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Print device information based on the provided property matrix.
   *
   * This function prints device information based on the provided property matrix. However,
   * the C++ accelerator back-end does not have the means to query the number of accelerator properties,
   * so the property matrix is not used in this function.
   * This function is still implemented here to have a unified structure with the other backends.
   * @param propertyMatrix The property matrix (unused).
   */
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);

  /**
   * @brief Test the kernel launch capability of the system.
   *
   * This function tests the kernel launch capability of the system.
   * The runtime check is not necessary as `_NVHPC_STDPAR_GPU` indicated we have C++.
   * GPU offloading available
   *
   * @return A vector indicating the kernel launch capability.
   */
  std::vector<bool> testKernelLaunch();

  /**
   * @brief Run benchmarks with the C++ backend.
   *
   * This function runs benchmarks using the C++ backend. However,
   * the C++ backend does not expose devices, so the benchmarks are executed in a unified manner
   * and the results contain only one GPU.
   *
   * Although not explicitly done, the first touch in the GPU kernel also involves copying the data over to the GPU.
   * And the second touch works on the data that already resides in the GPU, this way we can reproduce the streaming
   * benchmark for the other backends almost exactly.
   * We allocate roughly 1 GB of data on the CPU then transfer it to the GPU, and add half of this
   * data to the other half and transfer it back. Every step is then measured and saved in the BenchmarkResultsInNanoSeconds
   * struct to be printed as a table.
   * 
   * @return A vector containing the benchmark results.
   * 
   * @see tarch::accelerator::omp::runBenchmarks
   * @see tarch::accelerator::cuda::runBenchmarks
   * @see tarch::accelerator::sycl::runBenchmarks
   */
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::cpp
