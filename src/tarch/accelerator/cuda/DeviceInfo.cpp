#include "DeviceInfo.h"

#if defined(WITH_CUDA_API)
#include <cuda_runtime.h>
#include <iostream>
#include <sstream>

#include "../DeviceInfoUtility.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

namespace tarch::accelerator::info::cuda
{

  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    std::stringstream ss;
    int               numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    unsigned int                          propertyCount = tarch::accelerator::info::utility::properties.size();
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
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Device Name")][i] = props.name;
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Compute Architecture")][i]
      //  = std::to_string(props.major) + std::to_string(props.minor);
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Global Memory")][i]
        = std::to_string(props.totalGlobalMem / mb) + " mb";
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Shared Memory")][i]
      //  = std::to_string(props.sharedMemPerBlock / kb) + " kb";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Free Memory")][i] = std::to_string(free / mb)
                                                                                               + " mb";
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Constant Memory")][i]
      //  = std::to_string(props.totalConstMem / kb) + " kb";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Clock Frequency")][i]
        = std::to_string(props.clockRate / 1000) + " mHz";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Compute Units"
      )][i]
        = std::to_string(props.multiProcessorCount);
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Subgroup Size (Warp Size)")][i] = std::
      //  to_string(props.warpSize);
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Work Item Sizes (Block Size)")][i] = std::
      //  string(
      //    "<" + std::to_string(props.maxThreadsDim[0]) + ", " + std::to_string(props.maxThreadsDim[1]) + ", "
      //    + std::to_string(props.maxThreadsDim[2]) + ">"
      //  );
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Threads per Work Item")][i] = std::to_string(
      //  props.maxThreadsPerBlock
      //);
      //propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Grid Size")][i] = std::string(
      //  "<" + std::to_string(props.maxGridSize[0]) + ", " + std::to_string(props.maxGridSize[1]) + ", "
      //  + std::to_string(props.maxGridSize[2]) + ">"
      //);
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
      deviceInfoStr   = tarch::accelerator::info::utility::parseDeviceInformation(
        propertyMatrix, numberOfDevices
      );
    }
    std::cout << deviceInfoStr << std::endl;
  }

} // namespace tarch::accelerator::info::cuda

#endif