// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2 {
  INLINE_METHOD GPU_CALLABLE_METHOD int serialiseVoxelIndexInOverlap(
    const tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    int                                       numberOfDofsPerAxis,
    int                                       overlap,
    int                                       normal
  );
} // namespace exahype2
TARGET_DECLARE_END

#include "exahype2/Enumeration.cpph"
