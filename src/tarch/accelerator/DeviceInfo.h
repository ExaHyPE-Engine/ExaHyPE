#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  /**
   * Vector of strings representing various properties.
   *
   * This variable is a constant vector of strings that stores the names of various properties used to
   * identify the properties of GPUs.
   *
   * Using a struct per Device Could be better, but this way it is easier to create a Matrix with more Freedom
   * For cleaner code it can be a future todo since there is only 1 table being printed (initial design was to
   * have multiple remnants and it is a remnant of that design)
   *
   * @note This vector is intended to be used in conjunction with other functions or classes
   *       that require a list of properties.
   */
  const std::vector<std::string> properties = {
    "Device Name",
    "Global Memory",
    "Free Memory",
    "Clock Frequency",
    "Compute Units",
    "GPU-GPU Copy"};

  /**
   * @brief Struct representing benchmark results in nanoseconds.
   *
   * This is an internal struct holds the benchmark results for various operations
   * for streaming benchmarks measured in nanoseconds.
   * It includes the following fields and should ne needed by the end-user:
   *
   * - CPUtoGPUCopy: Time taken for CPU to GPU data copy.
   * - GPUtoCPUCopy: Time taken for GPU to CPU data copy.
   * - GPUMalloc: Time taken for GPU memory allocation.
   * - GPUtoGPUCopy: Time taken for GPU to GPU data copy.
   * - repeats: Number of repetitions performed for the benchmark.
   * - allocatedDataSize: Size of allocated data in bytes.
   * - readWriteSize: Size of data read/written during the benchmark.
   *
   * @note The values of these fields are obtained during benchmarking operations.
   */
  struct BenchmarkResultInNanoSeconds
  {
    uint64_t CPUtoGPUCopy;
    uint64_t GPUtoCPUCopy;
    uint64_t GPUMalloc;
    uint64_t GPUtoGPUCopy;
    uint64_t repeats;
    uint64_t allocatedDataSize;
    uint64_t readWriteSize;
  };

  /**
   * Num of repeats of streaming benchmarks and the number of elements N per vector
   */
  constexpr uint64_t repeats    = 10;
  constexpr size_t   alloc_size = 1024*1024*1024;
  constexpr size_t   N          = alloc_size / sizeof(double); // 1GB, Ensure the number divides the block size 256 without remainder for CUDA backend

  static tarch::logging::Log _log("tarch::accelerator");

  /**
   * Queries device information and runs benchmarks on the available accelerators.
   *
   * This function retrieves information about the available accelerators and performs
   * benchmarking using the appropriate backend based on the preprocessor directives.
   * The device information is stored in a vector of vectors of strings, where each
   * inner vector represents the information of a single property (A column in the Matrix then
   * belongs to one device, and one row belongs to one Property).
   *
   * Is the CUDA API is available it will be always prefered as it provides the most information
   * alongside sycl. As of July 23, ISO C++ and OpenMP GPU Offloading hides almost any information
   * available from the user
   *
   * @return The device information as a Matrix (vector<vector>) of strings.
   * @see tarch::accelerator::cpp::queryDeviceInformation
   * @see tarch::accelerator::cuda::queryDeviceInformation
   * @see tarch::accelerator::sycl::queryDeviceInformation
   * @see tarch::accelerator::omp::queryDeviceInformation
   */
  std::vector<std::vector<std::string>> queryDeviceInformation();

  /**
   * Prints device information based on the available accelerator backend.
   *
   * This function retrieves device information using the `queryDeviceInformation`
   * and `runBenchmarks`functions, it prints the information using the appropriate
   * backend based on the preprocessor directives (same backend as the  `queryDeviceInformation`).
   * Individual implementations are necessary because some backends hide the number of accelerators
   * from the user, this renders a table output useless and therefore individual decisions are necessary.
   *
   * @see queryDeviceInformation
   * @see tarch::accelerator::cpp::printDeviceInformation
   * @see tarch::accelerator::cuda::printDeviceInformation
   * @see tarch::accelerator::sycl::printDeviceInformation
   * @see tarch::accelerator::omp::printDeviceInformation
   */
  void printDeviceInformation();

  /**
   * Retrieves the offset of a property in a vector of identifiers.
   * By default the static vector of strings within this namespace is used
   *
   * This function searches for the given property name in the vector of identifiers.
   * If the property name is found, the function returns its offset (index) in the vector.
   * If the property name is not found, a runtime_error is thrown.
   *
   * @param name The name of the property to retrieve the offset for.
   * @param identifiers The vector of identifiers containing the property names.
   * @return The offset (index) of the property in the vector of identifiers.
   * @throws std::runtime_error If the property name is not found in the vector of identifiers.
   */
  size_t getPropertyOffset(const char name[], const std::vector<std::string>& identifiers = properties);

  /**
   * Parses a matrix of properties into a formatted string representation.
   *
   * This function takes a matrix of properties, the number of devices, and a vector of identifiers.
   * It formats the properties into a string representation, including headers and proper padding.
   * In the matrix, each column should have the properties from a device, the order should be same as the
   * tarch::accelerator::properties.
   *
   * @note The function is intended to use with `queryDeviceInformation()`and `parseBenchmarkResults(...)`
   *
   * @param propertyMatrix The matrix of properties to parse.
   * @param numberOfDevices The number of devices.
   * @param identifiers The vector of identifiers containing property names.
   * @return The formatted string representation of the property matrix.
   */
  std::string parseMatrix(
    const std::vector<std::vector<std::string>>& propertyMatrix,
    int                                          numberOfDevices,
    const std::vector<std::string>&              identifiers = properties
  );

  /**
   * Performs offload capability tests for the available GPU accelerator backend.
   *
   * The specific tests are executed using the respective
   * backend's `testKernelLaunch` function. The test results are evaluated to determine if the GPU
   * kernels can be successfully launched. The launched kernels are dummy (hello world) kernels just
   * to check whether launch fails or not.
   *
   * @throws std::runtime_error If the offload capability tests fail.
   *
   * @see tarch::accelerator::cpp::offloadCapabilityTest
   * @see tarch::accelerator::cuda::offloadCapabilityTest
   * @see tarch::accelerator::sycl::offloadCapabilityTest
   * @see tarch::accelerator::omp::offloadCapabilityTest
   *
   */
  void offloadCapabilityTest();

  /**
   * Runs benchmarks for the available GPU accelerator backend and populates the property matrix.
   * The propertyMatrix assumed to be used the matrix initialized properly (this is done by `queryBenchmarks`)
   *
   * Every backend implements (for their respective backends) a vector copy benchmark, which is memory bound.
   * The vector B is assigned to vector A where both of them are roughly 1 GB in size. The benchmarks perform
   * 10 repeats, where the result of the first repeat is discarded and the remaining 9 benchmarks' results are
   * averaged.
   *
   * @param propertyMatrix The matrix to store the benchmark results.
   *
   * @see tarch::accelerator::cpp::runBenchmarks
   * @see tarch::accelerator::cuda::runBenchmarks
   * @see tarch::accelerator::sycl::runBenchmarks
   * @see tarch::accelerator::omp::runBenchmarks
   */
  void runBenchmarks(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Parses benchmark results and updates the string representations in the property matrix.
   *
   * This function takes a vector of benchmark results (one result struct per GPU)
   * and updates the corresponding string representations in the already initialized propertyMatrixs
   *
   *
   * @param benchmarkResults The vector of benchmark results to parse.
   * @param benchmarkResultsStringRepresentations The property matrix to update with the string representations.
   */
  void parseBenchmarkResults(
    std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults, std::vector<std::vector<std::string>>& propertyMatrix
  );
} // namespace tarch::accelerator
