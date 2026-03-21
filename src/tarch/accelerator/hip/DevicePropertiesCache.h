// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_HIP)

#include <hip/hip_runtime.h>
#include <map>
#include <mutex>

namespace tarch::accelerator::hip {
  class DevicePropertiesCache;
} // namespace tarch::accelerator::hip

class tarch::accelerator::hip::DevicePropertiesCache {
public:
  static hipDeviceProp_t& get();

private:
  std::mutex                     _mtx;
  std::map<int, hipDeviceProp_t> _devicePropCache;
};

#endif
