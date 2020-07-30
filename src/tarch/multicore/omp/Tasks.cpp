#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"

#ifdef SharedOMP

#include <thread>
#include <queue>
#include "../Tasks.h"

#include "tarch/logging/Statistics.h"


namespace {
  std::priority_queue<
    tarch::multicore::Task*,
    std::vector<tarch::multicore::Task*>,
    tarch::multicore::TaskComparison
  > nonblockingTasks;

  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;
}


/**
 * Process a few tasks from my backlog of tasks and eventually trigger further consumes.
 *
 * After we have run through the given number of tasks, we examine whether we have
 * processed something (or have been called with argument 0) or the number of tasks
 * lingering in the background has even increased. In these cases, we span new OpenMP
 * tasks with low priority that continue to run through these pending jobs.
 */
bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier,        tarch::multicore::getNumberOfPendingTasks() );

  bool  result        = false;
  bool  spawnConsumer = maxTasks==0;
  const int   backupOfMaxTasks = maxTasks;

  while (maxTasks>0) {
    Task* myTask = nullptr;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (nonblockingTasks.empty()) {
        maxTasks = 0;
      }
      else {
        myTask = nonblockingTasks.top();
        nonblockingTasks.pop();
      }
    }

    if (myTask!=nullptr) {
      bool requeue = myTask->run();
      if (requeue) {
        spawnTask( myTask );
      }
      else {
        delete myTask;
      }
      maxTasks--;
      result = true;
      spawnConsumer = true;
    }
  }

  if (spawnConsumer) {
    int NumberOfTasksToSpawn = static_cast<int>(nonblockingTasks.size())>backupOfMaxTasks ? 2 : 1;
    for (int i=0; i<NumberOfTasksToSpawn; i++) {
      #pragma omp task firstprivate(backupOfMaxTasks) priority(BackgroundConsumerPriority)
      {
        ::tarch::logging::Statistics::getInstance().log( TasksPerConsumerRunStatisticsIdentifier, backupOfMaxTasks+1 );
        processPendingTasks(backupOfMaxTasks+1);
      }
    }
  }

  return result;
}


/**
 * Spawns a single task in a non-blocking fashion
 *
 * Non-blocking means that the routine should return immediately. One might
 * think that this is a great match for OpenMP tasks. However, spawnTasks()
 * is called by a taskgroup. New OpenMP tasks thus would become descendants
 * and eventually have to complete before the taskgroup completes. This is
 * not what we want: If we spawn a task this way, it really should go into
 * the background and be processed any time later when cores become idle.
 * So I simply enqueue these tasks in a local helper data structure and
 * return for the time being. The actual processing then is done via
 * processPendingTasks(). The latter is invoked before we hit a taskgroup
 * region the next time (so the processing of the things we call job here
 * can fill up idling threads), or it can be manually triggered by the code
 * (prior to MPI calls, e.g.).
 */
void tarch::multicore::spawnTask(Task*  job) {
  #pragma omp critical (backgroundTaskQueue)
  {
    nonblockingTasks.push(job);
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}



/**
 * Process a set of tasks and wait for their completion
 *
 * I originally thought we could just work with parallel fors. However, this
 * does not work, as we don't want to spawn our own parallel region here. We
 * expect the overarching main loop to be a parallel region. This main things
 * has to be embedded into a single/master statement, so it is only executed
 * on one thread. As a result, by the time we hit spawnAndWait, we are working
 * within a single environment. If we spawn parallel for within a single
 * environment, OpenMP will complain. It won't work, as we operate within
 * single. So I rely here on explicit tasking. Other threads will steal away
 * the guys we create.
 *
 * In principle, the processing of the tasks is a simple task group. However,
 * we may assume that that tasks are pretty imbalanced, so it would be nice
 * if the processing of some background tasks could slot in. Therefore, I
 * spawn another tasks that runs through all of the workitems that are still
 * pending in the background before I hit the parallel environment.
 *
 * @see  processPendingTasks(int).
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  processPendingTasks(0);

  #pragma omp taskgroup
  {
    for (int i=0; i<tasks.size(); i++) {
      #pragma omp task priority(StandardPriority)
      {
        while (tasks[i]->run()) {}
        delete tasks[i];
      }
    }
  }
}


void tarch::multicore::yield() {
  #pragma omp taskyield
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}


#endif
