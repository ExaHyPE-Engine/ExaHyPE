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
   * Covers a D-dimensional index space. The kernel receives D indices.
   */
  template <int D>
  struct RangeMD {
    static_assert(D == 2 or D == 3, "A multidimensional parallel loop supports 2D and 3D only.");
    int n[D];

    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny) :
      n{nx, ny} {
      static_assert(D == 2, "Only a 2D parallel loop takes two extents.");
    }
    GPU_CALLABLE_METHOD constexpr RangeMD(int nx, int ny, int nz) :
      n{nx, ny, nz} {
      static_assert(D == 3, "Only a 3D parallel loop takes three extents.");
    }
  };

  /**
   * Covers n independent teams. A GPU backend maps a team onto a thread block or a work group. The
   * kernel of a team calls parallelFor() or parallelReduce() with the team and an inner Range or
   * RangeMD.
   */
  struct TeamRange {
    static constexpr int BackendDefault = -1;

    int n;
    int threads;
    int scratchBytes;

    GPU_CALLABLE_METHOD constexpr explicit TeamRange(int size) :
      n(size),
      threads(BackendDefault),
      scratchBytes(BackendDefault) {}
    GPU_CALLABLE_METHOD constexpr TeamRange(int size, int threadsPerTeam, int bytesOfTeamScratch) :
      n(size),
      threads(threadsPerTeam),
      scratchBytes(bytesOfTeamScratch) {}
  };
} // namespace tarch::accelerator
