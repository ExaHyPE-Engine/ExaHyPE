// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_SYCL_SYCL_H_
#define _TARCH_MULTICORE_SYCL_SYCL_H_


#include <string>
#include "config.h"


/**
 * @page SYCL SYCL support
 *
 * Using SYCL in Peano can be tricky, as SYCL's interface (headers, e.g.) still
 * changes from time to time, and as SYCL uses a couple of macros and functions
 * that we use in Peano, too. Examples are Dimensions and assertions.
 *
 * Therefore, I provide a dedicated SYCL.h header in tarch/multicore/tbb which
 * wraps around the standard SYCL headers. I strongly recommend all Peano files
 * to use this header rather than SYCL directly.
 *
 * Besides the fancy includes, the SYCL header also provides the getSYCLQueue()
 * routine. When you start up the Peano's Core object, it will automatically
 * put one SYCL queue onto each device. You can query the singleton for the
 * number of devices from anywhere in the code, but to get hold of these
 * devices, you should use the getter in the SYCL.h header.
 *
 *
 * ## Implementation
 *
 * For all macros which are used both in Peano and in SYCL, I use the pragmas
 * push_macro and pop_macro to temporarily undefine them while I include the
 * SYCL headers, and to bring them back into the game afterwards.
 *
 * ## Dependencies
 *
 * Don't include any other Peano header in this file, as all Peano headers might
 * indirectly include this one via multicore.h, as they need the macro
 * SYCL_EXTERNAL.
 *
 * The only header you need from Peano is the actual config.h.
 *
 */

#if defined(GPUOffloadingSYCL) or defined(SharedSYCL)


#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")


namespace tarch {
  namespace multicore {
    /**
     * Get a SYCL queue
     *
     * The Core singleton instantiates all SYCL queues. You can query the
     * singleton for the total number of (logical) devices. Per device,
     * we hold one SYCL queue persistently. You can gain access to this
     * one queue via this getter.
     */
    sycl::queue& getSYCLQueue( int number );

    /**
     * Return number of SYCL queues
     *
     * The default host queue is not counted here, so you won't get the queue
     * labelled by HostDefaultQueue.
     */
    int getNumberOfSYCLQueues();

    void initSYCLQueues();

    constexpr int HostDefaultQueue = -1;
  }
}


#endif
#endif
