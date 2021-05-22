#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"


#include "tarch/logging/Statistics.h"



#if defined(SharedOMP)

namespace {
  int nonblockingTasks                 = 0;

  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;

  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");


  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    // important, otherwise taskwait synchronises
    assertion( not tasks.empty() );

    #pragma omp taskloop nogroup priority(StandardPriority) untied
    for (int i=0; i<static_cast<int>(tasks.size()); i++) {
      ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
      while (tasks[i]->run()) {}
      delete tasks[i];
      ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
    }
    #pragma omp taskwait // wait for all elements from tasks to complete
                         // do not wait for the children of tasks
                         // but do children if you have time (this does not
                         // happen however)
  }


  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    assertion( not tasks.empty() );
    assertion( tarch::multicore::getRealisation()!=tarch::multicore::Realisation::MapOntoNativeTasks );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );
    int       busyThreads     = NumberOfThreads;

    // for task loop, I need an explicit shared(busyThreads)
    for (int i=0; i<NumberOfThreads; i++) {
      #pragma omp task shared(busyThreads)
      {
        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
        if (i<tasks.size()) {
          while (tasks[i]->run()) {
            #pragma omp taskyield
          }
          delete tasks[i];
        }

        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
  
        #pragma omp atomic
        busyThreads--;

        while (
          busyThreads>0
          and
          // Without this yield, the code will deadlock if you have more trees
          // than cores. As the first p trees on p cores will finish and then
          // poll. The other >p trees/tasks will starve
          busyThreads<tarch::multicore::Core::getInstance().getNumberOfThreads()
          and
          tarch::multicore::getRealisation()!=tarch::multicore::Realisation::HoldTasksBackInLocalQueue
        ) {
          tarch::multicore::processPendingTasks( 1 );
          #pragma omp taskyield
        }
      }
    }
    #pragma omp taskwait
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  #pragma omp atomic
  nonblockingTasks++;

  #pragma omp task priority(BackgroundConsumerPriority) untied
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
 * Use a taskloop and label it as nogroup, i.e. we only synchronise over the
 * siblings. If the loop bodies to yield further tasks, we do not wait for
 * these guys.
 *
 * If I remove the group label from the taskloop, i.e. if I use nogroup and
 * thus do not ask OpenMP to make the for loop a taskgroup, then I have no
 * synchronisation left over et al. However, I want a sync over all the tasks
 * produced by the loop. Therefore, I have to add a manual taskwait.
 */
void tarch::multicore::native::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  switch (tarch::multicore::getRealisation()) {
    case Realisation::MapOntoNativeTasks:
      spawnAndWaitAsTaskLoop(tasks);
      break;
    case Realisation::HoldTasksBackInLocalQueue:
    case Realisation::HoldTasksBackInLocalQueueAndBackfill:
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfill:
    case Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask:
    case Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask:
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask:
      spawnAndWaitAsExplicitTasksWithPolling(tasks);
      break;
  }
}


void tarch::multicore::native::yield() {
  #pragma omp taskyield
}


int tarch::multicore::native::getNumberOfPendingTasks() {
  return nonblockingTasks;
}


#endif
