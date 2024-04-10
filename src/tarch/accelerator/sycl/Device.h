// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "config.h"


/**
 * @page tarch_accelerator_SYCL SYCL support
 *
 * This page provides a programmer's view on SYCL. If you search for details
 * for a particular software stack/compiler settings, these are can either be
 * found @ref page_compiler_specific_settings "on the generic vendor pages" or
 * you might find SYCL details on the pages for
 * @ref page_machines "particular machines".
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
 *
 * ## Implementation
 *
 * For all macros which are used both in Peano and in SYCL, I use the pragmas
 * push_macro and pop_macro to temporarily undefine them while I include the
 * SYCL headers, and to bring them back into the game afterwards.
 *
 *
 * ## Dependencies
 *
 * Don't include any other Peano header in the file tarch/accelerator/sycl/Device.h,
 * as all Peano headers might
 * indirectly include this one via multicore.h (as they need the macro
 * SYCL_EXTERNAL, e.g.). The only header that this file in turn needs
 * from Peano is the actual config.h.
 *
 *
 * ## SYCL impact on other source code files
 *
 * Once we use code in SYCL constructs, we have to take a few constraints into
 * account:
 *
 * - No assertions in the code.
 * - All functions used have to be marked as SYCL_EXTERNAL, unless they are
 *   defined in the header.
 * - No string output via output streams.
 *
 * Therefore, a lot of routines in the tarch::la component have ifdefs that
 * explicitly mask out assertions. They are usually templates and therefore in
 * headers anyway.
 *
 * A further complication arises from the fact that Peano also can use SYCL
 * on the CPU as multicore front-end. There, I tend to use d-dimensional
 * for loops quite a lot, as they are defined in peano4/utils/Loop.h. These
 * loops are realised as macros, which in turn use functions from the header.
 * These functions have to be marked as SYCL functions now, too, even though
 * they are only used on the CPU. peano4:utils::dLinearised() provides some
 * docu.
 *
 * Even one step further, the functions rely heavily on the tarch's linear
 * algebra. Therefore, the linear algebra subcomponent has to disable all the
 * problematic flavours from the list above if we use SYCL on the GPU
 * ***or on the CPU***. It does not matter.
 *
 *
 * ## SYCL on the host
 *
 * SYCL can be used on the host such that SYCL handles the multicore
 * parallelism. In this case, I use one SYCL queue on the CPU which is
 * administered in tarch/multicore/sycl/Core.h. The key ingredients there
 * are
 *
 * - an internal (hidden) queue object for the host, and
 * - the function tarch::multicore::getHostSYCLQueue().
 *
 * When we initialise the host queue, we have to be little bit careful:
 * Newer versions of SYCL need a selector for the queue. Otherwise SYCL will
 * use heuristics and likely deploy our queue to the GPU. The docu is not
 * consistent. A lot of (older) docu says the host would be the default
 * fall-back, but I cannot trust that here.
 *
 * You can construct a CPU selector object manually, but there should already
 * be such a variable which is globally defined.
 *
 * The "old" syntax
 *
 * ~~~~~~~~~~~~~~~~~~
 * sycl::queue  hostSyclQueue( sycl::device(sycl::cpu_selector()) );
 * ~~~~~~~~~~~~~~~~~~
 *
 * seems not to work anymore.
 *
 * On top of that, we need to consider that the tarch is shipped as static
 * object. Therefore, it can happen that a global queue object is created
 * before SYCL is properly initialised. This leads to a seg fault. What I
 * do now is that the global queue object results from the default constructor.
 * Once the tarch::multicore::Core object is instantiated - this will happen
 * after Peano is properly initialised - its constructor creates a "proper"
 * SYCL queue on the CPU host and overwrites the global queue object.
 *
 * Without this overwrite/reinitialisation, I permanently ended up in seg
 * faults. Please note that any output in the Core constructor has to be
 * realised through cout/cerr, as the logging might not be up. However, to bring
 * the logging up, it will again try to instantiate the Core object to
 * find out which core wrote the error message. So we have to circumnavigate
 * Peano's logging.
 */
#if defined(GPUOffloadingSYCL) or defined(SharedSYCL)

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
