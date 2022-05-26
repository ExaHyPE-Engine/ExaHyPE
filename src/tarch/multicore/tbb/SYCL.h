// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_TBB_SYCL_H_
#define _TARCH_MULTICORE_TBB_SYCL_H_

#include <string>
#include "tarch/multicore/multicore.h"


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
 *
 */

#if GPUOffloadingSYCL>0

//#define _FUNCTEXCEPT_H 1

/*
namespace std {
  #pragma omp declare target
  //void __throw_bad_function_call() {};
  [[noreturn]] void __throw_bad_function_call() { __builtin_trap(); };
  #pragma omp  end declare target
}
*/


#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")


namespace tarch {
  namespace multicore {
    sycl::queue& getSYCLQueue( int number );
  }
}


#endif
#endif
