// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(SharedOMP)

#define MK_DEBUG 0
#if MK_DEBUG
#include <stdio.h>
#endif

namespace tarch::accelerator::omp {
  static inline void* mk_omp_get_mapped_ptr(const void* ptr, int targetDevice) {
  #if defined(__AMDGPU__) or defined(__clang__)
    uintptr_t resolved;
    #pragma omp target map(from:resolved) device(targetDevice)
    {
      resolved = reinterpret_cast<uintptr_t>(ptr);
    }
  #if MK_DEBUG
    fprintf(stderr, "mk_omp_get_mapped_ptr: returning %p for device %d, host pointer %p\n", reinterpret_cast<void *>(resolved), targetDevice, ptr);
  #endif
    return reinterpret_cast<void *>(resolved);
  #else
    return mk_omp_get_mapped_ptr(ptr, targetDevice);
  #endif
  }
}

#endif // SharedOMP
