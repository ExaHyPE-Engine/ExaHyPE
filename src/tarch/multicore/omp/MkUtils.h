// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_OMP_MKUTILS_H_
#define _TARCH_MULTICORE_OMP_MKUTILS_H_

#if defined(SharedOMP)

#define MK_DEBUG 0
#if MK_DEBUG
#include <stdio.h>
#endif

static inline
void * mk_omp_get_mapped_ptr(const void * ptr, int device_num) {
    uintptr_t resolved;
    #pragma omp target map(from:resolved)
    {
        resolved = reinterpret_cast<uintptr_t>(ptr);
    }
#if MK_DEBUG
    fprintf(stderr, "mk_omp_get_mapped_ptr: returning %p for device %d, host pointer %p\n", reinterpret_cast<void *>(resolved), device_num, ptr);
#endif
    return reinterpret_cast<void *>(resolved);
}

#endif // SharedOMP

#endif // _TARCH_MULTICORE_OMP_MKUTILS_H_
