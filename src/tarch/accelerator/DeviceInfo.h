// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  /**
   * Vector of strings representing various properties.
   *
   * This variable is a constant vector of strings that stores the
   * names of various properties used to identify the properties of the accelerators.
   */
  const std::vector<std::string> properties = {"Device Name", "Global Memory", "Free Memory", "Clock Frequency", "Compute Units", "Bandwidth"};

  /**
   * @brief Struct representing benchmark results in nanoseconds.
   *
   * This is an internal struct holding the benchmark results for various operations
   * for streaming benchmarks measured in nanoseconds.
   *
   * @note The values of these fields are obtained during benchmarking operations.
   */
  struct BenchmarkResultInNanoSeconds {
    std::uint64_t CPUtoGPUCopy;
    std::uint64_t GPUtoCPUCopy;
    std::uint64_t GPUMalloc;
    std::uint64_t GPUtoGPUCopy;
    std::uint64_t repeats;
    std::uint64_t allocatedDataSize;
    std::uint64_t readWriteSize;
  };

  constexpr std::uint64_t NumberOfRepeats = 10;
  constexpr std::size_t   AllocSize       = 1024 * 1024 * 1024;
  constexpr std::size_t   N               = AllocSize / sizeof(double);

  static tarch::logging::Log _log("tarch::accelerator");

  /**
   * Queries device information and runs benchmarks on the available accelerators.
   *
   * This function retrieves information about the available accelerators and performs
   * benchmarking using the selected accelerator implementation based on the preprocessor directives.
   * The device information is stored in a vector of vectors of strings, where each
   * inner vector represents the information of a single property (A column in the matrix then
   * belongs to one device, and one row belongs to one property).
   */
  std::vector<std::vector<std::string>> queryDeviceInformation();

  /**
   * Prints device information based on the selected accelerator implementation.
   *
   * @see queryDeviceInformation
   */
  void printDeviceInformation();

  /**
   * Retrieves the offset of a property in a vector of identifiers.
   * By default the static vector of strings within this namespace is used
   *
   * This function searches for the given property name in the vector of identifiers.
   * If the property name is found, the function returns its offset (index) in the vector.
   * If the property name is not found, a runtime error is thrown.
   *
   * @param name The name of the property to retrieve the offset for.
   * @param identifiers The vector of identifiers containing the property names.
   * @return The offset (index) of the property in the vector of identifiers.
   * @throws std::runtime_error If the property name is not found in the vector of identifiers.
   */
  std::size_t getPropertyOffset(const char name[], const std::vector<std::string>& identifiers = properties);

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
  std::string parseMatrix(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices, const std::vector<std::string>& identifiers = properties);

  /**
   * Performs offload capability tests for the selected accelerator implementation.
   *
   * @throws std::runtime_error If the offload capability tests fail.
   *
   */
  void offloadCapabilityTest();

  /**
   * Runs benchmarks for the selected accelerator implementation and populates the property matrix.
   * The property matrix assumed to be used the matrix initialised properly (this is done by `queryBenchmarks`).
   *
   * Each accelerator implementation implements a vector copy benchmark, which is memory bound.
   * The vector B is assigned to vector A where both of them are roughly 1.0 GB in size. The benchmarks perform
   * 'NumberOfRepeats' repeats, where the result of the first repeat is discarded and the remaining results are
   * averaged.
   *
   * @param propertyMatrix The matrix to store the benchmark results.
   */
  void runBenchmarks(std::vector<std::vector<std::string>>& propertyMatrix);

  /**
   * Parses benchmark results and updates the string representations in the property matrix.
   *
   * This function takes a vector of benchmark results (one result struct per accelerator)
   * and updates the corresponding string representations in the already initialised property matrix.
   *
   * @param benchmarkResults The vector of benchmark results to parse.
   * @param benchmarkResultsStringRepresentations The property matrix to update with the string representations.
   */
  void parseBenchmarkResults(std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults, std::vector<std::vector<std::string>>& propertyMatrix);
} // namespace tarch::accelerator
