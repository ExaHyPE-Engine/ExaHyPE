// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

#include <omp.h>

namespace tarch::accelerator {
  int getNumberOfScratchChunks(int workItems);
  INLINE_METHOD GPU_CALLABLE_METHOD int getScratchChunk(int workItem);
} // namespace tarch::accelerator

#include "tarch/accelerator/Scratch.cpph"
