#include "DeviceInfo.h"

#if defined(GPUOffloadingOMP)
#include <iostream>
#include <omp.h>

#include "../DeviceInfoUtility.h"

namespace tarch::accelerator::info::omp
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    unsigned int                          propertyCount   = tarch::accelerator::info::utility::properties.size();
    unsigned int                          numberOfDevices = omp_get_num_devices();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto        propertyMatrix    = queryDeviceInformation();
    std::string ompBackendWarning = "OpenMP accelerator back-end has no means to query the accelerators or properties\n"
                                    "For best results enable the CUDA or Sycl Backends/API.\n";
    std::string deviceInfoStr     = numberOfDevices + " offloading capable GPUs present." 
    // This table will be empty
    //std::string deviceInfoStr     = tarch::accelerator::info::utility::parseDeviceInformation(propertyMatrix, 0);
    std::cout << ompBackendWarning << deviceInfoStr << std::endl;
  }
} // namespace tarch::accelerator::info::omp

#endif