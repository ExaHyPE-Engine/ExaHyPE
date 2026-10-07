// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
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
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/Device.h"
#endif

namespace tarch::accelerator {
  template <typename Functor>
  INLINE_METHOD void parallelForOnHost(const Range& range, Functor functor);

#if (defined(PEANO_WITH_GPU_BACKEND_CUDA) and not defined(__CUDACC__)) \
  or (defined(PEANO_WITH_GPU_BACKEND_HIP) and not defined(__HIPCC__))
  // The CUDA and HIP backends define these templates in their device translation units only. A
  // host translation unit calls them through declarations that are not inline.
  template <typename Functor>
  void parallelFor(const Range& range, Functor functor);
  template <typename Functor>
  void parallelFor(const RangeMD<2>& range, Functor functor);
  template <typename Functor>
  void parallelFor(const RangeMD<3>& range, Functor functor);
  template <typename Functor>
  void parallelFor(const TeamRange& range, Functor functor);
#endif

  INLINE_METHOD int getTeamThreads(int requested);
  INLINE_METHOD int getTeamScratchBytes(int requested);

#if (not defined(PEANO_WITH_GPU_BACKEND_CUDA) and not defined(PEANO_WITH_GPU_BACKEND_HIP)) or defined(__CUDACC__) \
  or defined(__HIPCC__)
  TARGET_DECLARE
  INLINE_METHOD GPU_CALLABLE_METHOD int getThreadOfTeam(const TeamHandle& team);
  INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfThreadsPerTeam(const TeamHandle& team);
  INLINE_METHOD GPU_CALLABLE_METHOD unsigned char* getTeamScratch(const TeamHandle& team);
  INLINE_METHOD GPU_CALLABLE_METHOD void teamBarrier(const TeamHandle& team);
  TARGET_DECLARE_END
#endif
#if not defined(PEANO_WITH_GPU_BACKEND)
  INLINE_METHOD unsigned char* getTeamScratchOfThread();
#endif
} // namespace tarch::accelerator

#include "tarch/accelerator/ParallelForOnHost.cpph"

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
