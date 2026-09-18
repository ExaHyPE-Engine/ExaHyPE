// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/accelerator/ParallelRegion.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch::accelerator {
  template <typename Reducer, typename F>
  void parallelReduce(const Range& range, F body, Reducer reducer);
  template <typename Reducer, typename F>
  void parallelReduce(const RangeMD<2>& range, F body, Reducer reducer);
  template <typename Reducer, typename F>
  void parallelReduce(const RangeMD<3>& range, F body, Reducer reducer);
  template <typename Reducer, typename F>
  void parallelReduce(const TeamRange& range, F body, Reducer reducer);
} // namespace tarch::accelerator

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Device.h"
#include "tarch/accelerator/kokkos/ParallelReduce.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/cuda/Device.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/accelerator/cuda/ParallelReduce.cpph"

#include <cub/cub.cuh>
#include <cuda_runtime.h>
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/hip/Device.h"
#include "tarch/accelerator/hip/ErrorCheck.h"
#include "tarch/accelerator/hip/ParallelReduce.cpph"

#include <hip/hip_runtime.h>
#include <hipcub/hipcub.hpp>
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h"
#include "tarch/accelerator/sycl/ParallelReduce.cpph"

#include <sycl/sycl.hpp>
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ompt/ParallelReduce.cpph"
#else
#include "tarch/accelerator/ParallelReduce.cpph"
#endif
