// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2 {
  INLINE_METHOD GPU_CALLABLE_METHOD int serialiseVoxelIndexInOverlap(
    const tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    int                                       numberOfDofsPerAxis,
    int                                       overlap,
    int                                       normal
  );
} // namespace exahype2

#include "exahype2/Enumeration.cpph"
