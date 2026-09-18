// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace tarch::accelerator {
  /**
   * Flat 1-dimensional range [0, n).
   * Maps to OMP `for (int i=0; i<n; ++i)`, Kokkos::RangePolicy(0,n),
   * CUDA/HIP grid stride, sycl::range<1>(n).
   */
  struct Range {
    int n;
    GPU_CALLABLE_METHOD constexpr explicit Range(int n_) :
      n(n_) {}
  };

  /**
   * Multi-dimensional range, D in {2, 3}.
   * Lambda receives D ints (e.g. `[=](int x, int y, int z){...}` for D=3).
   */
  template <int D>
  struct RangeMD {
    static_assert(D == 2 or D == 3, "RangeMD only supports D=2 or D=3");
    int n[D];

    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny) :
      n{nx, ny} {
      static_assert(D == 2, "2-arg RangeMD ctor requires D==2");
    }
    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny, int nz) :
      n{nx, ny, nz} {
      static_assert(D == 3, "3-arg RangeMD ctor requires D==3");
    }
  };

  /**
   * Outer hierarchical range: `n` independent teams (Kokkos teams / CUDA blocks /
   * SYCL groups). Inside the lambda you call `tarch::accelerator::parallelFor(team, ...)` or
   * `tarch::accelerator::parallelReduce(team, ...)` with an inner Range/RangeMD.
   */
  struct TeamRange {
    int n;
    GPU_CALLABLE_METHOD constexpr explicit TeamRange(int n_) :
      n(n_) {}
  };
} // namespace tarch::accelerator
