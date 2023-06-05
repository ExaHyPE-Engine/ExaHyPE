#include "DeviceInfo.h"

#if defined(GPUOffloadingCPP)
#include <iostream>

#include "../DeviceInfoUtility.h"

namespace tarch::accelerator::info::cpp
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    unsigned int                          propertyCount = tarch::accelerator::info::utility::properties.size();
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
} // namespace tarch::accelerator::info::cpp
#endif