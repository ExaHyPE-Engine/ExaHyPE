// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch {
  template <typename Reducer, typename F>
  void parallelReduce(const Range& r, F f, Reducer red);
  template <typename Reducer, typename F>
  void parallelReduce(const RangeMD<2>& r, F f, Reducer red);
  template <typename Reducer, typename F>
  void parallelReduce(const RangeMD<3>& r, F f, Reducer red);
  template <typename Reducer, typename F>
  void parallelReduce(const TeamRange& r, F f, Reducer red);
} // namespace tarch

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/ParallelReduce.cpph"
#else
#include "tarch/accelerator/ParallelReduce.cpph"
#endif

namespace tarch {
  using accelerator::parallelReduce;
} // namespace tarch
