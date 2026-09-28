// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace tarch::accelerator {
  /**
   * Covers the work items 0 to n - 1 of a one-dimensional dispatch.
   */
  struct Range {
    int n;
    GPU_CALLABLE_METHOD constexpr explicit Range(int size) :
      n(size) {}
  };

  /**
   * Covers a D-dimensional index space, and the kernel receives D indices.
   */
  template <int D>
  struct RangeMD {
    static_assert(D == 2 or D == 3, "A RangeMD has two or three dimensions.");
    int n[D];

    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny) :
      n{nx, ny} {
      static_assert(D == 2, "The constructor with two extents requires D == 2.");
    }
    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny, int nz) :
      n{nx, ny, nz} {
      static_assert(D == 3, "The constructor with three extents requires D == 3.");
    }
  };

  /**
   * Covers n independent teams, which a GPU backend maps onto thread blocks or work groups. The
   * kernel of one team calls parallelFor() or parallelReduce() with the team and an inner Range or
   * RangeMD.
   */
  struct TeamRange {
    int n;
    GPU_CALLABLE_METHOD constexpr explicit TeamRange(int size) :
      n(size) {}
  };
} // namespace tarch::accelerator
