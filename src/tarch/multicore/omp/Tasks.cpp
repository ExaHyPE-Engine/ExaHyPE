#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"

#ifdef SharedOMP

#include <thread>
#include <queue>
#include "../Tasks.h"

#include "tarch/logging/Statistics.h"



#if defined(OpenMPManualTaskPriorityImplementation)
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
 *
 * <h2> Task group </h2>
 *
 * If I do not handle the last task explicitly on the master, I ran into the
 * situation that I had n tasks in tasks, but OpenMP used n+1 threads with the
 * master not doing any work. This severely limited scalability.
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  if (not tasks.empty()) {
    #pragma omp taskgroup
    {
      for (int i=1; i<tasks.size(); i++) {
        #pragma omp task
        {
          while (tasks[i]->run()) {}
          delete tasks[i];
        }
      }
      while (tasks[0]->run()) {}
      delete tasks[0];
    }
  }
}


void tarch::multicore::yield() {
  #pragma omp taskyield
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#elif !defined(OpenMPManualTaskPriorityImplementation)
namespace {
  int numberOfNonblockingTasks;
}


/**
 * To teh best of my knowledge, there's no way to tell OpenMP directly that I
 * wanna do up to maxTasks directly and then return. So I have this crude
 * workaround.
 */
bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);

  int previousNumberOfTasks = tarch::multicore::getNumberOfPendingTasks();
  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, previousNumberOfTasks );

  if (maxTasks>0) {
    #pragma omp taskyield
  }

  return tarch::multicore::getNumberOfPendingTasks() < previousNumberOfTasks;
}


void tarch::multicore::spawnTask(Task*  task) {
  #pragma omp atomic
  numberOfNonblockingTasks += 1;

  #pragma omp task priority( task->getPriority() )
  {
    bool requeue = task->run();
    if (requeue) {
      spawnTask( task );
    }
    else {
      delete task;
    }

    #pragma omp atomic
    numberOfNonblockingTasks -= 1;
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}


/**
 * In theory, a task can requeue itself. Shouldn't do so in this context though.
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  #pragma omp taskgroup
  {
    for (int i=1; i<tasks.size(); i++) {
      #pragma omp task priority( tasks[i]->getPriority() )
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


/**
 * @return Number of background tasks that are still out there
 */
int tarch::multicore::getNumberOfPendingTasks() {
  return numberOfNonblockingTasks;
}

#endif

#endif
