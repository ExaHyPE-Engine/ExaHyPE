// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DevicePropertiesCache.h"

#if defined(GPUOffloadingCUDA)

#include "ErrorCheck.h"
#include "tarch/Assertions.h"

cudaDeviceProp& tarch::accelerator::cuda::DevicePropertiesCache::get() {
  static DevicePropertiesCache instance;

  int         devId  = -1;
  CHECK_CUDA_ERROR(cudaGetDevice(&devId));
  assertion(devId >= 0);

  std::lock_guard<std::mutex> guard(instance._mtx);

  if (instance._devicePropCache.find(devId) == instance._devicePropCache.end()) {
    // Cache miss
    instance._devicePropCache[devId] = cudaDeviceProp();
    CHECK_CUDA_ERROR(cudaGetDeviceProperties(&instance._devicePropCache[devId], devId));
  }

  return instance._devicePropCache[devId];
}

#endif
