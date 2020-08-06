#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"

#if defined(SharedOMP) and !defined(LayeredMultitaskingRuntime)


namespace {
  int nonblockingTasks                 = 0;

  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;
}


/**
 * No (implicit) synchronisation applied here, so spawnTask() returns
 * immediately. The taskyield ensures that reoccuring jobs do not block the
 * execution.
 */
void tarch::multicore::spawnTask(Task*  job) {
  #pragma omp atomic
  nonblockingTasks++;

  #pragma omp task priority(BackgroundConsumerPriority)
  {
    while (job->run()) {
      #pragma omp taskyield
    }
    delete job;
    #pragma omp atomic
    nonblockingTasks--;
  }
}


/**
 * Use a taskloop and label is as nogroup, i.e. we only synchronise over the
 * siblings. If the loop bodies to yield further tasks, we do not wait for
 * these guys.
 *
 * If I remove the group label from the taskloop, i.e. if I use nogroup and
 * thus do not ask OpenMP to make the for loop a taskgroup, then I have no
 * synchronisation left over et al. However, I want a sync over all the tasks
 * produced by the loop. Therefore, I have to add a manual taskwait.
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  #pragma omp taskloop nogroup priority(StandardPriority)
  for (int i=0; i<tasks.size(); i++) {
    while (tasks[i]->run()) {}
    delete tasks[i];
  }
  #pragma omp taskwait
}


bool tarch::multicore::processPendingTasks(int) {
  int previousTasks = nonblockingTasks;
  yield();
  return previousTasks > nonblockingTasks;
}


void tarch::multicore::yield() {
  #pragma omp taskyield
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks;
}


#endif
