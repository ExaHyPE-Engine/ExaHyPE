// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Scratch.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"

#include <omp.h>

/**
 * Returns the number of scratch chunks a dispatch over workItems needs. A GPU backend needs one
 * chunk per work item, the host one per thread of the team that runs the dispatch. A caller
 * allocates the chunks at the nesting level of the dispatch.
 */
int
tarch::accelerator::getNumberOfScratchChunks(const int workItems) {
#if defined(PEANO_WITH_GPU_BACKEND)
  return workItems;
#else
  return std::max(1, omp_in_parallel() ? omp_get_num_threads() : omp_get_max_threads());
#endif
}

/**
 * Returns the number of teams of a dispatch whose team entry runs the work items entry, entry +
 * teams and so on. The host runs one team per work item. A device runs at most as many teams of
 * teamThreads threads as it keeps resident at once. Every dispatch runs at least one team.
 */
int
tarch::accelerator::getNumberOfPersistentTeams(const int workItems, const int teamThreads) {
  assertion(workItems >= 0, workItems);
  assertion(teamThreads > 0, teamThreads);
#if defined(PEANO_WITH_GPU_BACKEND)
  const int concurrentTeams = tarch::accelerator::Device::getInstance().getNumberOfConcurrentThreads() / teamThreads;
  return std::max(1, std::min(workItems, concurrentTeams));
#else
  return std::max(1, workItems);
#endif
}
