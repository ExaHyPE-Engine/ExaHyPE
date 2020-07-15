#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"

#ifdef SharedOMP

#include <thread>
#include <queue>
#include "../Tasks.h"

#include "tarch/logging/Statistics.h"


namespace {
  std::priority_queue<tarch::multicore::Task* > nonblockingTasks;
}

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
    int NumberOfTasksToSpawn = nonblockingTasks.size()>backupOfMaxTasks ? 2 : 1;
    for (int i=0; i<NumberOfTasksToSpawn; i++) {
      #pragma omp task firstprivate(backupOfMaxTasks)
      {
        ::tarch::logging::Statistics::getInstance().log( TasksPerConsumerRunStatisticsIdentifier, backupOfMaxTasks+1 );
        processPendingTasks(backupOfMaxTasks+1);
      }
    }
  }

  return result;
}


void tarch::multicore::spawnTask(Task*  job) {
  #pragma omp critical (backgroundTaskQueue)
  {
    nonblockingTasks.push(job);
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}



/**
 * I originally thought we could just work with parallel fors. However, this
 * does not work, as we don't want to spawn our own parallel region here. We
 * expect the overarching main loop to be a parallel region. This main things
 * has to be embedded into a single/master statement, so it is only executed
 * on one thread. As a result, by the time we hit spawnAndWait, we are working
 * within a single environment. If we spawn parallel for within a single
 * environment, OpenMP will complain. It won't work, as we operate within
 * single. So I rely here on explicit tasking.
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  #pragma omp taskgroup
  {
    for (int i=0; i<tasks.size(); i++) {
      #pragma omp task
      {
        while (tasks[i]->run()) {}
        delete tasks[i];
      }
    }
  }
}


/**
 * This has to be the standard C++ implementation. Even if multithreading is
 * disabled, the code might logically run with multiple threads, and then we
 * have to switch the thread context from time to time.
 */
void tarch::multicore::yield() {
  std::this_thread::yield();
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#endif
