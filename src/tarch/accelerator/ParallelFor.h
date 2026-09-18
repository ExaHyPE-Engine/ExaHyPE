// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/accelerator/ParallelRegion.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch::accelerator {
  template <typename F>
  void parallelFor(const Range& range, F body);
  template <typename F>
  void parallelFor(const RangeMD<2>& range, F body);
  template <typename F>
  void parallelFor(const RangeMD<3>& range, F body);
  template <typename F>
  void parallelFor(const TeamRange& range, F body);
} // namespace tarch::accelerator

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Device.h"
#include "tarch/accelerator/kokkos/ParallelFor.cpph"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/Device.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/accelerator/cuda/ParallelFor.cpph"

#include <cuda_runtime.h>
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/Device.h"
#include "tarch/accelerator/hip/ErrorCheck.h"
#include "tarch/accelerator/hip/ParallelFor.cpph"

#include <hip/hip_runtime.h>
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h"
#include "tarch/accelerator/sycl/ParallelFor.cpph"

#include <sycl/sycl.hpp>
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/ParallelFor.cpph"
#else
#include "tarch/accelerator/ParallelFor.cpph"
#endif
