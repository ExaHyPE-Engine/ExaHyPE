// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator {
  void setDeviceStream(hipStream_t stream);
  hipStream_t getDeviceStream();

  // The team overloads read threadIdx and blockDim.
  struct HipTeam final {};
  using TeamHandle = HipTeam;

  inline constexpr int HipBlockSize         = 256;
  inline constexpr int HipMaxNumberOfBlocks = 65535;
} // namespace tarch::accelerator

#endif
