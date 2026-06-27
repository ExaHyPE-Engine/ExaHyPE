// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch {
  template <typename F>
  void parallelFor(const Range& r, F f);
  template <typename F>
  void parallelFor(const RangeMD<2>& r, F f);
  template <typename F>
  void parallelFor(const RangeMD<3>& r, F f);
  template <typename F>
  void parallelFor(const TeamRange& r, F f);
} // namespace tarch

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/ParallelFor.cpph"
#else
#include "tarch/accelerator/ParallelFor.cpph"
#endif

namespace tarch {
  using accelerator::parallelFor;
} // namespace tarch
