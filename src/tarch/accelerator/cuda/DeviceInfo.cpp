#include "DeviceInfo.h"

#if defined(WITH_CUDA_API)
#include <cuda_runtime.h>
#include <iostream>
#include <sstream>

#include "../DeviceInfo.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

std::vector<bool> testKernelLaunchCUDACall();
namespace tarch::accelerator::info::cuda
{

  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    std::stringstream ss;
    int               numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    unsigned int                          propertyCount = tarch::accelerator::info::properties.size();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));

    int defaultDevice;
    cudaGetDevice(&defaultDevice);
    for (unsigned int i = 0; i < numberOfDevices; i++)
    {
      cudaSetDevice(i);
      cudaDeviceProp props;
      cudaGetDeviceProperties(&props, i);
      size_t free, total;
      cudaMemGetInfo(&free, &total);
      constexpr unsigned int kb                                                              = 1024;
      constexpr unsigned int mb                                                              = kb * kb;
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Device Name")][i] = props.name;
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Global Memory")][i]
        = std::to_string(props.totalGlobalMem / mb) + " mb";
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Free Memory")][i] = std::to_string(free / mb)
                                                                                               + " mb";
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Clock Frequency")][i]
        = std::to_string(props.clockRate / 1000) + " mHz";
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Compute Units"
      )][i]
        = std::to_string(props.multiProcessorCount);
    }
    cudaSetDevice(defaultDevice);
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto propertyMatrix = queryDeviceInformation();
    int  numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    std::string deviceInfoStr   = "";
    if (numberOfDevices == 0){
      deviceInfoStr   = "No offloading capable GPUs were found";
    }else{
      deviceInfoStr   = tarch::accelerator::info::parseDeviceInformation(
        propertyMatrix, numberOfDevices
      );
    }
    std::cout << deviceInfoStr << std::endl;
  }

  std::vector<bool> testKernelLaunch() {
    return testKernelLaunchCUDACall();
  }
} // namespace tarch::accelerator::info::cuda

#endif