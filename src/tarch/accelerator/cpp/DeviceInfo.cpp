#include "DeviceInfo.h"

#if defined(GPUOffloadingCPP)
#include <iostream>

#include "../DeviceInfo.h"

namespace tarch::accelerator::info::cpp
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    unsigned int                          propertyCount = tarch::accelerator::info::properties.size();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(0, "X"));
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto        propertyMatrix    = queryDeviceInformation();
    std::string cppBackendWarning = "Iso cpp accelerator back-end has no means to query the number of accelerators or "
                                    "properties\n"
                                    "For best results enable the CUDA or Sycl Backends/API.\n";
    std::cout << cppBackendWarning << std::endl;
  }

  std::vector<bool> testKernelLaunch() {
    #if !defined(_NVHPC_STDPAR_GPU)
    return std::vector<bool>(1, false);
    #else
    std::vector<bool> v;
    return v;
    #endif
  }

  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks() {
    return std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds>();
  }
} // namespace tarch::accelerator::info::cpp
#endif