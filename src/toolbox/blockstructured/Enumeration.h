// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace toolbox::blockstructured {
  /**
   * The volumes or elements within an overlap are always enumerated
   * lexicographically. This routine serialises this index.
   *
   * @param overlap Overlap of one patch into the other. If you have a
   *   halo of one cell around each patch, then this parameter is 1.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int serialiseVoxelIndexInOverlap(
    const tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    int                                       numberOfDofsPerAxis,
    int                                       overlap,
    int                                       normal
  );
} // namespace toolbox::blockstructured

#include "Enumeration.cpph"
