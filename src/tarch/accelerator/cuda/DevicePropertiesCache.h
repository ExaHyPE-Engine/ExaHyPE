// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(GPUOffloadingCUDA)

#include <cuda_runtime.h>
#include <map>
#include <mutex>

namespace tarch::accelerator::cuda {
  class DevicePropertiesCache;
} // namespace tarch::accelerator::cuda

class tarch::accelerator::cuda::DevicePropertiesCache {
public:
  static cudaDeviceProp& get();

private:
  std::mutex                    _mtx;
  std::map<int, cudaDeviceProp> _devicePropCache;
};

#endif
