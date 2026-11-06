// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DevicePropertiesCache.h"

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/hip/ErrorCheck.h"

hipDeviceProp_t&
tarch::accelerator::hip::DevicePropertiesCache::getProperties() {
  static DevicePropertiesCache instance;

  int deviceId = -1;
  CHECK_HIP_ERROR(hipGetDevice(&deviceId));
  ASSERTION(deviceId >= 0);

  std::lock_guard<std::mutex> guard(instance._mtx);

  if (instance._devicePropCache.find(deviceId) == instance._devicePropCache.end()) {
    instance._devicePropCache[deviceId] = hipDeviceProp_t();
    CHECK_HIP_ERROR(hipGetDeviceProperties(&instance._devicePropCache[deviceId], deviceId));
  }

  return instance._devicePropCache[deviceId];
}

#endif
