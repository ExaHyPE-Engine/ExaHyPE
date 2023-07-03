#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"
namespace tarch::accelerator
{
  // Using a struct per Device Could be easier, but this way it is easier to create a Matrix with more Freedom
  // For a cleaner code it can be a future todo since there is only 1 table being printed
  /**
   * @brief Vector containing properties of a device.
   *
   * This vector holds the names of various properties related to a device.
   * It is used to unify the communication between functions used in device information and should not 
   * be needed by the users
   *
   */
  const std::vector<std::string> properties = {
      "Device Name",
      "Global Memory",
      "Free Memory",
      "Clock Frequency",
      "Compute Units",
      "Streaming Bandwidth",
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

  static tarch::logging::Log _log("tarch::accelerator");
  
  /**
   * @brief Queries device information for available accelerators (gpus).
   *
   * This function queries device information for available accelerators using
   * the specified backend at configuration time. 
   * The device information is stored in a nested vector
   * structure, where each inner vector represents the information for a single
   * device and contains strings describing various properties.
   * 
   * The matrix / vector structure uses a static const vector for indexing between 
   * various functions
   *
   * @return A vector of vectors containing device information.
   *
   * @note The backend to be used is determined based on preprocessor directives.
   *       If `_NVHPC_CUDA` is defined even though there might be other backends we do cuda calls
   *       as CUDA gives the most useful information to us, SYCL also gives enough information
   *       in ISO CPP and OpenMP we get almost no infromation about the accelerators.
   *
   * @note This function also runs benchmarks on the available devices.
   */
  std::vector<std::vector<std::string>> queryDeviceInformation();

/**
 * @brief Prints device information for available accelerators.
 *
 * This function retrieves device information using the `queryDeviceInformation`
 * function and then prints the information using the specified backend. The
 * device information is passed as a matrix of strings.
 *
 * @note the bachend choice happens the same as in `queryDeviceInformation`
 */
  void printDeviceInformation();

  /**
   * @brief Get the offset of a property within a list of identifiers, intended to be used with `properties`.
   *
   * This function searches for the specified property name within a vector of identifiers and returns its offset.
   *
   * @param name The name of the property to find.
   * @param identifiers The vector of property identifiers default is `properties`.
   * @return The offset of the property within the identifiers vector.
   *
   * @throws std::runtime_error if the property name is not found in the identifiers list.
   *
   * @note The identifiers vector should contain unique property names.
   */
  size_t getPropertyOffset(const char name[], const std::vector<std::string> &identifiers = properties);

/**
 * @brief Parse a property matrix into a formatted string Where every column should be properties of the device.
 *
 * This function takes a property matrix, number of devices, and a vector of identifiers and converts them into a formatted string representation.
 * Every column of the matrix should contain the properties of the device. The order should be the same as the `properties` vector used.
 * In the output the row 0 has then the identifiers of the `properties` and the column 0 has the enumeration of the devices. The table output is also padded
 * so that every entry has the same length in a table cell. (The input of the matrix has device properties per column with each row for a different identifier 
 * this means the input will be transposed during parsing on the fly)
 * 
 * @param propertyMatrix The property matrix to parse.
 * @param numberOfDevices The number of devices.
 * @param identifiers The vector of property identifiers, default is the `properties` from the namespace.
 * @return A formatted string representing the property matrix.
 *
 * @note The propertyMatrix should be a 2D vector containing the property values for each device and obtained by `queryDeviceInformation`.
 * @note The identifiers vector should contain the names of the properties.
 *
 * @see getPropertyOffset
 * @see queryDeviceInformation
 */
  std::string parseMatrix(const std::vector<std::vector<std::string>> &propertyMatrix, int numberOfDevices, const std::vector<std::string> &identifiers = properties);

  /**
   * @brief Perform offload capability test for GPU acceleration.
   *
   * This function tests the capability of the GPU acceleration backend to successfully launch GPU kernels.
   *
   * @note The function will log information depending on the capabiliteis of the backend
   * @note The function throw runtime errors if it can't launch kernels.
   *
   * @see tarch::accelerator::cuda::testKernelLaunch
   * @see tarch::accelerator::sycl::testKernelLaunch
   * @see tarch::accelerator::omp::testKernelLaunch
   * @see tarch::accelerator::cpp::testKernelLaunch
   */
  void offloadCapabilityTest();

  /**
   * @brief Run benchmarks for GPU acceleration and update the property matrix.
   *
   * This function runs benchmarks for GPU acceleration based on the defined GPU acceleration backend.
   * It stores the benchmark results in the `results` vector and updates the `propertyMatrix` accordingly
   * using the `parseBenchmarkResults` function. This function works on a matrix that is partially written
   * from the `queryDeviceInformation`. Results for device with id i is written to the column i of the matrix
   * As we print a single table but might use two different backends to obtain the device information and
   * benchmarks this approach is necessary.
   * 
   * For example compiling for the OpenMP GPU Offloading backend with NVHPC compilers means that we will have 
   * CUDA API available and thus get good information on the devices but we will still run the benchmarks with
   * OpenMP backend as it will be the one using in the simulation.
   *
   * @param[in,out] propertyMatrix A matrix of property values to be updated with benchmark results.
   *
   * @note The function executes the appropriate benchmark function based on the defined GPU acceleration backend.
   *       The benchmark results are then parsed and stored in the `propertyMatrix`.
   *
   * @see tarch::accelerator::cuda::runBenchmarks
   * @see tarch::accelerator::sycl::runBenchmarks
   * @see tarch::accelerator::omp::runBenchmarks
   * @see tarch::accelerator::cpp::runBenchmarks
   * @see parseBenchmarkResults
   * @see queryDeviceInformation
   */
  void runBenchmarks(std::vector<std::vector<std::string>> &propertyMatrix);

/**
 * @brief Parse the benchmark results and update the property matrix.
 *
 * This function parses the benchmark results stored in the `benchmarkResults` vector and updates
 * the `propertyMatrix` accordingly. The benchmark results are expected to be of type `BenchmarkResultInNanoSeconds`
 *
 * @param[in] benchmarkResults A vector of benchmark results to be parsed.
 * @param[in,out] propertyMatrix A matrix of property values to be updated with the parsed results.
 *
 * @note The function iterates over the benchmark results and extracts the relevant information to update
 *       the corresponding entries in the `propertyMatrix`.
 *
 * @see BenchmarkResultInNanoSeconds
 */
  void parseBenchmarkResults(std::vector<BenchmarkResultInNanoSeconds> &benchmarkResults, std::vector<std::vector<std::string>> &propertyMatrix);
} // namespace tarch::accelerator
