// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/accelerator/cuda/ErrorCheck.h"

#include <cuda_runtime.h>

namespace tarch::accelerator {
  void setDeviceStream(cudaStream_t stream);
  cudaStream_t getDeviceStream();

  // The team overloads read threadIdx and blockDim.
  struct CudaTeam final {};
  using TeamHandle = CudaTeam;

  inline constexpr int CudaBlockSize         = 256;
  inline constexpr int CudaMaxNumberOfBlocks = 65535;
} // namespace tarch::accelerator

#endif
