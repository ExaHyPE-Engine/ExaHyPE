// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_HIP)

#include "ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator {
  hipStream_t deviceStreamCreate();
  void        deviceStreamDestroy(hipStream_t*);
  void        deviceStreamSync(hipStream_t);

  void        deviceStreamSet(hipStream_t);
  hipStream_t deviceStreamGet();
} // namespace tarch::accelerator

#endif
