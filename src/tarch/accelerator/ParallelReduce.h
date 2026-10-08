// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelRegion.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Device.h"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#if defined(__CUDACC__)
#include "tarch/accelerator/cuda/Device.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"

#include <cuda_runtime.h>
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#if defined(__HIPCC__)
#include "tarch/accelerator/hip/Device.h"
#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h"

#include <sycl/sycl.hpp>
#endif

namespace tarch::accelerator {
#if (defined(PEANO_WITH_GPU_BACKEND_CUDA) and not defined(__CUDACC__)) \
  or (defined(PEANO_WITH_GPU_BACKEND_HIP) and not defined(__HIPCC__))
  // The CUDA and HIP backends define these templates in their device translation units only. A
  // host translation unit calls them through declarations that are not inline.
  template <typename Reducer, typename Functor>
  void parallelReduce(const Range& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  void parallelReduce(const RangeMD<2>& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  void parallelReduce(const RangeMD<3>& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  void parallelReduce(const TeamRange& range, Functor functor, Reducer reducer);
#endif

#if (not defined(PEANO_WITH_GPU_BACKEND_CUDA) and not defined(PEANO_WITH_GPU_BACKEND_HIP)) or defined(__CUDACC__) \
  or defined(__HIPCC__)
  TARGET_DECLARE
  template <typename Reducer>
  INLINE_METHOD GPU_CALLABLE_METHOD void reduceOverTeam(
    const TeamHandle&           team,
    typename Reducer::ValueType threadValue,
    Reducer                     reducer
  );
  TARGET_DECLARE_END
#endif

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
  template <typename Reducer>
  INLINE_METHOD GPU_CALLABLE_METHOD typename Reducer::ValueType reduceTeamValue(
    const tarch::accelerator::SyclTeam& team,
    typename Reducer::ValueType         value,
    Reducer                             reducer
  );

  template <typename Op>
  INLINE_METHOD GPU_CALLABLE_METHOD typename Op::ValueType reduceTeamValue(
    const tarch::accelerator::SyclTeam& team,
    typename Op::ValueType              value,
    tarch::accelerator::Custom<Op>      reducer
  );
#endif
} // namespace tarch::accelerator

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#if defined(__CUDACC__)
#include "tarch/accelerator/cuda/ParallelReduce.cpph"
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#if defined(__HIPCC__)
#include "tarch/accelerator/hip/ParallelReduce.cpph"
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/ParallelReduce.cpph"
#else
#include "tarch/accelerator/ParallelReduce.cpph"
#endif
