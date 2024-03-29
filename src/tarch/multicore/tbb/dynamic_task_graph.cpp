#include "dynamic_task_graph.h"

//#if defined(SharedTBB)


/**
 * Describes a task
 *
 * Describes a task which is pending or runnning, i.e. has been spawned. Each
 * such task knows
 *
 * - the actual task to be executed;
 * - how many in-dependencies it has;
 * - what the actual task does.
 *
 * If the number of in-dependencies becomes 0, the task is ready, i.e. we can
 * hand it over to TBB.
 * has its own mutex, so we can lock
 */
/*
struct SpawnedTaskDescriptor {
  tarch::multicore::Task* task;
  std::atomic<int>        numberOfPredecessorTasks;
  std::list<int>          successorTasks;
  std::mutex              mutex;
};
*/


/*
getArena(job)->enqueue([&, job] {
  while (job->run()) {
  }
  delete job;
});
*/

//#endif
