// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Scratch.h"

#include <omp.h>

/**
 * Returns the number of scratch chunks that a dispatch over workItems needs, so that
 * getScratchChunk() of every running work item names its own chunk. A GPU backend needs one chunk
 * per work item, and the host needs one chunk per thread of the team that runs the dispatch: The
 * team of the enclosing parallel region, or the region the dispatch opens outside of one. A
 * caller allocates the chunks at the nesting level of the dispatch.
 */
int
tarch::accelerator::getNumberOfScratchChunks(const int workItems) {
#if defined(PEANO_WITH_GPU_BACKEND)
  return workItems;
#else
  return std::max(1, omp_in_parallel() ? omp_get_num_threads() : omp_get_max_threads());
#endif
}
