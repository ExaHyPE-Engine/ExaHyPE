// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DevicePropertiesCache.h"

#if defined(WITH_GPU_HIP)

#include "ErrorCheck.h"
#include "tarch/Assertions.h"

hipDeviceProp_t& tarch::accelerator::hip::DevicePropertiesCache::get() {
  static DevicePropertiesCache instance;

  int devId = -1;
  CHECK_HIP_ERROR(hipGetDevice(&devId));
  assertion(devId >= 0);

  std::lock_guard<std::mutex> guard(instance._mtx);

  if (instance._devicePropCache.find(devId) == instance._devicePropCache.end()) {
    // Cache miss
    instance._devicePropCache[devId] = hipDeviceProp_t();
    CHECK_HIP_ERROR(hipGetDeviceProperties(&instance._devicePropCache[devId], devId));
  }

  return instance._devicePropCache[devId];
}

#endif
