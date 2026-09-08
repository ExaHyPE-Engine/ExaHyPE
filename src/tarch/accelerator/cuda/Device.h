// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/accelerator/cuda/ErrorCheck.h"

#include <cuda_runtime.h>

namespace tarch::accelerator {
  cudaStream_t deviceStreamCreate();
  void         deviceStreamDestroy(cudaStream_t* stream);
  void         deviceStreamSync(cudaStream_t stream);

  void         deviceStreamSet(cudaStream_t stream);
  cudaStream_t deviceStreamGet();

  // Opaque team handle. CUDA carries no state on the team; intra-block context
  // (threadIdx, blockDim) is read directly inside the inner overloads.
  struct CudaTeam {};
  using TeamHandle = CudaTeam;

  inline constexpr int CudaBlockSize = 256;
} // namespace tarch::accelerator

#endif
