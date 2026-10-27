// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator {
  void setDeviceStream(hipStream_t stream);
  hipStream_t getDeviceStream();

  // The team overloads read threadIdx and blockDim. The scratch is the shared memory of the block.
  struct HipTeam final {
    unsigned char* scratch{nullptr};
  };
  using TeamHandle = HipTeam;

  inline constexpr int HipBlockSize         = 256;
  inline constexpr int HipMaxNumberOfBlocks = 65535;
  inline constexpr int ThreadsPerWarp       = 64;
  // A team launch runs one block per team of at most HipBlockSize threads with at most
  // TeamScratchBytes of shared memory. A launch with TeamRange::BackendDefault runs
  // DefaultThreadsPerTeam threads.
  inline constexpr int DefaultThreadsPerTeam = HipBlockSize;
  inline constexpr int TeamScratchBytes      = 16384;
} // namespace tarch::accelerator

#endif
