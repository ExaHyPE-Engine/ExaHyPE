#include "DeviceInfo.h"

#if defined(GPUOffloadingCPP)
#include <iostream>

#include "../DeviceInfo.h"

namespace tarch::accelerator::cpp
{
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix)
  {
    unsigned int propertyCount = tarch::accelerator::properties.size();
    std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(0, "X"));
    propertyMatrix = std::move(_propertyMatrix);
  }

  void printDeviceInformation([[maybe_unused]] std::vector<std::vector<std::string>> &propertyMatrix)
  {
    logInfo("printDeviceInformation()", "Iso cpp accelerator back-end has no means to query the number of accelerators' properties and run benchmarks");
  }

  std::vector<bool> testKernelLaunch()
  {
#if !defined(_NVHPC_STDPAR_GPU)
    return std::vector<bool>(1, false);
#else
    std::vector<bool> v;
    return v;
#endif
  }

  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks()
  {
    return std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds>();
  }
} // namespace tarch::accelerator::cpp
#endif