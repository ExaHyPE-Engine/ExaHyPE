// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DevicePropertiesCache.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/Assertions.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"

cudaDeviceProp& tarch::accelerator::cuda::DevicePropertiesCache::get() {
  static DevicePropertiesCache instance;

  int deviceId = -1;
  CHECK_CUDA_ERROR(cudaGetDevice(&deviceId));
  assertion(deviceId >= 0);

  std::lock_guard<std::mutex> guard(instance._mtx);

  if (instance._devicePropCache.find(deviceId) == instance._devicePropCache.end()) {
    instance._devicePropCache[deviceId] = cudaDeviceProp();
    CHECK_CUDA_ERROR(cudaGetDeviceProperties(&instance._devicePropCache[deviceId], deviceId));
  }

  return instance._devicePropCache[deviceId];
}

#endif
