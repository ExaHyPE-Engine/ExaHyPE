// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/accelerator/ParallelRegion.h"
#include "tarch/accelerator/Range.h"
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
  template <typename Functor>
  void parallelForOnHost(const Range& range, Functor functor);

  template <typename Functor>
  void parallelFor(const Range& range, Functor functor);
  template <typename Functor>
  void parallelFor(const RangeMD<2>& range, Functor functor);
  template <typename Functor>
  void parallelFor(const RangeMD<3>& range, Functor functor);
  template <typename Functor>
  void parallelFor(const TeamRange& range, Functor functor);
} // namespace tarch::accelerator

#include "tarch/accelerator/ParallelForOnHost.cpph"

// A CUDA or HIP kernel launch compiles in a translation unit of that language only, so a plain
// C++ translation unit of such a build defines parallelForOnHost alone.
#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#if defined(__CUDACC__)
#include "tarch/accelerator/cuda/ParallelFor.cpph"
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#if defined(__HIPCC__)
#include "tarch/accelerator/hip/ParallelFor.cpph"
#endif
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/ParallelFor.cpph"
#else
#include "tarch/accelerator/ParallelFor.cpph"
#endif
