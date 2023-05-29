#include "DeviceInfo.h"

#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
#include "cuda/DeviceInfo.h"
#elif defined(GPUOffloadingSYCL)
#include "sycl/DeviceInfo.h"
#elif defined(GPUOffloadingOMP)
#include "omp/DeviceInfo.h"
#elif defined(GPUOffloadingCPP)
#include "cpp/DeviceInfo.h"
#else
#include <iostream>
#endif

// #else if defined(WITH_SYCL_API) // Possible TODO to differentiate SYCL_API and Sycl

namespace tarch::accelerator::info
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
    return tarch::accelerator::info::cuda::queryDeviceInformation();
#elif defined(GPUOffloadingSYCL)
    return tarch::accelerator::info::sycl::queryDeviceInformation();
#elif defined(GPUOffloadingOMP)
    return tarch::accelerator::info::omp::queryDeviceInformation();
#elif defined(GPUOffloadingCPP)
    return tarch::accelerator::info::cpp::queryDeviceInformation();
#else
    std::vector<std::vector<std::string>> propertyMatrix;
    return propertyMatrix;
#endif
  }

  void printDeviceInformation()
  {
#if defined(WITH_CUDA_API) or defined(GPUOffloadingCUDA)
    return tarch::accelerator::info::cuda::printDeviceInformation();
#elif defined(GPUOffloadingSYCL)
    return tarch::accelerator::info::sycl::printDeviceInformation();
#elif defined(GPUOffloadingOMP)
    // An empty table, with OpenMP we will only know how many GPUs we have
    return tarch::accelerator::info::omp::printDeviceInformation();
#elif defined(GPUOffloadingCPP)
    // Can't get any information with cpp GPU offloading rn
    return tarch::accelerator::info::cpp::printDeviceInformation();
#else
    // Print noinformation since we do not have an accelerator
    std::cout << "No GPU Accelerator Backend is enabled. The code will run on CPUs." << std::endl;
#endif
  }
} // namespace tarch::accelerator::info
