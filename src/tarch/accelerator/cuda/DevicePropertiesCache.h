// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include <cuda_runtime.h>

namespace tarch::accelerator::cuda {
  class DevicePropertiesCache;
} // namespace tarch::accelerator::cuda

class tarch::accelerator::cuda::DevicePropertiesCache {
public:
  static cudaDeviceProp& getProperties();

private:
  std::mutex                    _mtx;
  std::map<int, cudaDeviceProp> _devicePropCache;
};

#endif
