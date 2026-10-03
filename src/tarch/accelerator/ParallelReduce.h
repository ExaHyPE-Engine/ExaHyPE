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

#include <cub/cub.cuh>
#include <cuda_runtime.h>
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#if defined(__HIPCC__)
#include "tarch/accelerator/hip/Device.h"
#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>
#include <hipcub/hipcub.hpp>
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h"

#include <sycl/sycl.hpp>
#endif

namespace tarch::accelerator {
  template <typename Reducer, typename Functor>
  INLINE_METHOD void parallelReduce(const Range& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  INLINE_METHOD void parallelReduce(const RangeMD<2>& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  INLINE_METHOD void parallelReduce(const RangeMD<3>& range, Functor functor, Reducer reducer);
  template <typename Reducer, typename Functor>
  INLINE_METHOD void parallelReduce(const TeamRange& range, Functor functor, Reducer reducer);

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
