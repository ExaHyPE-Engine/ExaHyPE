#include "DeviceInfo.h"

#if defined(GPUOffloadingOMP)
#include <iostream>
#include <omp.h>

#include "../DeviceInfo.h"
#include "Device.h"

namespace tarch::accelerator::info::omp
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    unsigned int                          propertyCount   = tarch::accelerator::info::properties.size();
    unsigned int                          numberOfDevices = omp_get_num_devices();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto        propertyMatrix    = queryDeviceInformation();
    int         numberOfDevices   = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::string ompBackendWarning = "OpenMP accelerator back-end has no means to query the accelerators or properties\n"
                                    "For best results enable the CUDA or Sycl Backends/API.\n";
    std::string deviceInfoStr     = numberOfDevices + " offloading capable GPUs present.";
    // This table will be empty
    //std::string deviceInfoStr     = tarch::accelerator::info::parseDeviceInformation(propertyMatrix, 0);
    std::cout << ompBackendWarning << deviceInfoStr << std::endl;
  }

  std::vector<bool> testKernelLaunch() {
    int numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::vector<bool> testResults(numberOfDevices, false);

    for (int i = 0; i< numberOfDevices; i++)
    {
      bool canOffload = false;
      #pragma omp target device(i) map(tofrom : canOffload)
      {
        if (!omp_is_initial_device()){
          canOffload = true;
        }
      }
      testResults[i] = canOffload;
    }

    return testResults;
  }
} // namespace tarch::accelerator::info::omp

#endif