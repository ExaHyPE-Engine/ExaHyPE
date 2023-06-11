#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info
{
  // Using a struct per Device Could be easier, but this way it is easier to create a Matrix with more Freedom
  // For a cleaner code it can be a future todo since there is only 1 table being printed
  const std::vector<std::string> properties = {
    "Device Name",
    "Global Memory",
    "Free Memory",
    "Clock Frequency",
    "Compute Units",
  };

  const std::vector<std::string> benchmarkIdentifiers = {
    "CPU-to-GPU Copy",
    "GPU-to-CPU Copy",
    "GPU-to-GPU Copy",
    "GPU-Malloc",
  };

  struct BenchmarkResultInNanoSeconds {
    uint64_t CPUtoGPUCopy;
    uint64_t GPUtoCPUCopy;
    uint64_t GPUMalloc;
    uint64_t GPUtoGPUCopy;
    uint64_t repeats;
    uint64_t allocatedDataSize;
    uint64_t copiedDataSize;
  };

  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();
  size_t                                getPropertyOffset(const char name[], const std::vector<std::string>& identifiers = properties);
  std::string                           parseMatrix(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices,  const std::vector<std::string>& identifiers = properties);
  void                                  offloadCapabilityTest();
  void                                  runBenchmarks();
  void                                  parseBenchmarkResults(std::vector<BenchmarkResultInNanoSeconds>& benchmarkResults);
} // namespace tarch::accelerator::info
