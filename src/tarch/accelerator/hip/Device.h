// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator {
  hipStream_t deviceStreamCreate();
  void        deviceStreamDestroy(hipStream_t* stream);
  void        deviceStreamSync(hipStream_t stream);

  void        deviceStreamSet(hipStream_t stream);
  hipStream_t deviceStreamGet();

  // HipTeam carries no state: The team overloads read threadIdx and blockDim directly.
  struct HipTeam final {};
  using TeamHandle = HipTeam;

  inline constexpr int HipBlockSize         = 256;
  inline constexpr int HipMaxNumberOfBlocks = 65535;
} // namespace tarch::accelerator

#endif
