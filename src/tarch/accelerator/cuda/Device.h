// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/accelerator/cuda/ErrorCheck.h"

#include <cuda_runtime.h>

namespace tarch::accelerator {
  void setDeviceStream(cudaStream_t stream);
  cudaStream_t getDeviceStream();

  // The team overloads read threadIdx and blockDim. The scratch is the shared memory of the block.
  struct CudaTeam final {
    unsigned char* scratch{nullptr};
  };
  using TeamHandle = CudaTeam;

  inline constexpr int CudaBlockSize         = 256;
  inline constexpr int CudaMaxNumberOfBlocks = 65535;
  inline constexpr int ThreadsPerWarp        = 32;
  // A team launch runs one block per team of at most CudaBlockSize threads with at most
  // TeamScratchBytes of shared memory. A launch with TeamRange::BackendDefault runs
  // DefaultThreadsPerTeam threads.
  inline constexpr int DefaultThreadsPerTeam = CudaBlockSize;
  inline constexpr int TeamScratchBytes      = 16384;
} // namespace tarch::accelerator

#endif
