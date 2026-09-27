// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Scratch.h"

#include <omp.h>

/**
 * Returns the number of scratch chunks that a dispatch over workItems needs, so that
 * getScratchChunk() of every running work item names its own chunk. A GPU backend needs one chunk
 * per work item, and the host needs one chunk per thread.
 */
int
tarch::accelerator::getNumberOfScratchChunks(const int workItems) {
#if defined(PEANO_WITH_GPU_BACKEND)
  return workItems;
#else
  return std::max(1, omp_get_max_threads());
#endif
}
