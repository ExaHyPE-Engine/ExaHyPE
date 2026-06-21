// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "ErrorCheck.h"

#include <cuda_runtime.h>

namespace tarch::accelerator {
  cudaStream_t deviceStreamCreate();
  void deviceStreamDestroy(cudaStream_t*);
  void deviceStreamSync(cudaStream_t);

  void deviceStreamSet(cudaStream_t);
  cudaStream_t deviceStreamGet();
} // namespace tarch::accelerator

#endif
