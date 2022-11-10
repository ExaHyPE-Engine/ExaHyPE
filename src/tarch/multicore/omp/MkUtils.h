// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_OMP_MKUTILS_H_
#define _TARCH_MULTICORE_OMP_MKUTILS_H_

#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(SharedOMP)

#define MK_DEBUG 0
#if MK_DEBUG
#include <stdio.h>
#endif

static inline
void * mk_omp_get_mapped_ptr(const void * ptr, int device_num) {
#if defined(__AMDGPU__) or defined(__clang__)
    uintptr_t resolved;
    #pragma omp target map(from:resolved) device(device_num)
    {
        resolved = reinterpret_cast<uintptr_t>(ptr);
    }
#if MK_DEBUG
    fprintf(stderr, "mk_omp_get_mapped_ptr: returning %p for device %d, host pointer %p\n", reinterpret_cast<void *>(resolved), device_num, ptr);
#endif
    return reinterpret_cast<void *>(resolved);
#else
    return omp_get_mapped_ptr(ptr, device_num);
#endif
}

#endif // SharedOMP

#endif // _TARCH_MULTICORE_OMP_MKUTILS_H_
