// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @page tarch_accelerator_SYCL SYCL support
 *
 * Using SYCL in Peano can be tricky, as SYCL's interface (headers, e.g.) still
 * changes from time to time, and as SYCL uses a couple of macros and functions
 * that we use in Peano, too. Examples are Dimensions and assertions.
 *
 * Therefore, I provide a dedicated SYCL.h header in tarch/multicore/sycl which
 * wraps around the standard SYCL headers. I strongly recommend all Peano files
 * to use this header rather than SYCL directly:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 * #include "tarch/multicore/sycl/Core.h"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Besides the fancy includes, the SYCL header also provides the tarch::accelerator::getSYCLQueue()
 * routine. When you start up the Peano's Core object, it will automatically
 * put one SYCL queue onto each device. You can query the singleton for the
 * number of devices from anywhere in the code, but to get hold of these
 * devices, you should use the getter in the SYCL.h header.
 *
 * ## Implementation
 *
 * For all macros which are used both in Peano and in SYCL, I use the pragmas
 * push_macro and pop_macro to temporarily undefine them while I include the
 * SYCL headers, and to bring them back into the game afterwards.
 *
 * ## Dependencies
 *
 * Don't include any other Peano header in the file tarch/accelerator/sycl/Device.h, as all Peano headers might
 * indirectly include this one via multicore.h (as they need the macro
 * SYCL_EXTERNAL, e.g.). The only header that this file in turn needs
 * from Peano is the actual config.h.
 *
 */
#if defined(GPUOffloadingSYCL)

#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")

#include <set>

namespace tarch {
  namespace accelerator {
    /**
     * Get a SYCL queue
     *
     * The Device singleton instantiates all SYCL queues. You can query the
     * singleton for the total number of (logical) devices. Per device,
     * we hold one SYCL queue persistently. You can gain access to this
     * one queue via this getter.
     *
     * number is a logical number, i.e. it is a number from 0 to
     * getNumberOfSYCLQueues(). The actual device number might be totally
     * different.
     *
     * If you use SYCL on the host, too, you should not access this queue
     * throught he accelerator namespace, but use the tarch::multicore
     * namespace.
     *
     * @see tarch::multicore::getHostSYCLQueue()
     */
    sycl::queue& getSYCLQueue( int number );

    /**
     * Write available queues to cout. The logging interface is not used, as
     * the logging might not be up when you invoke this routine for the first
     * time.
     */
    void listSYCLQueues();
  }
}

#endif
