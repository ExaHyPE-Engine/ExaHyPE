#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"

#include "tarch/mpi/Rank.h"

#include "tarch/logging/Statistics.h"


#if defined(SharedOMP)

namespace {
  int nonblockingTasksCounter          = 0;

  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;

  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");


  /**
   * Native mapping of a task loop onto a task loop
   *
   * I found that quite a lot of OpenMP implementations realise the taskwait at the 
   * end as busy wait. This is totally annoying as it might lead to deadlocks whenever
   * a task has a taskyield internally and/or there are more tasks than threads.
   */
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
      #pragma omp taskyield
    }
    #pragma omp taskwait // wait for all elements from tasks to complete
                         // do not wait for the children of tasks
                         // but do children if you have time (this does not
                         // happen however)
  }


  /**
   * This implementation variant follows the patterns described in the 2021
   * IWOMP paper by H. Schulz et al.
   *
   *    * IWOMP paper by H. Schulz et al. An important paper to read in the context
   * of the present solution is https://www.osti.gov/pages/servlets/purl/1465188.
   * Note in particular the example in Figure 3.


   *
   * NVIDIA does not support the taskloop
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    int       pendingTasks  = NumberOfThreads;
    int       spawnedTasks  = 0;

    //#pragma omp taskloop nogroup priority(StandardPriority) untied
    for (int i=0; i<NumberOfThreads; i++) {
      #pragma omp task shared(pendingTasks, spawnedTasks)
      {
        #pragma omp atomic
	spawnedTasks++;

        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
        if (i<tasks.size()) {
          while (tasks[i]->run()) {
            #pragma omp taskyield
          }
          delete tasks[i];
        }

        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
  
        #pragma omp atomic seq_cst
        pendingTasks--;

	//#pragma omp flush(busyThreads)

	//std::cout << "thread " << i << " reduced busy threads to " << busyThreads << std::endl;

        while (
          pendingTasks>0
          and
          // Without this yield, the code will deadlock if you have more trees
          // than cores. As the first p trees on p cores will finish and then
          // poll. The other >p trees/tasks will starve
          pendingTasks<tarch::multicore::Core::getInstance().getNumberOfThreads()
	  and 
	  spawnedTasks==NumberOfThreads
        ) {
		std::cout << "@@@@@ I'll try @@@@@@" << std::endl;
          int numberOfPendingTasks = tarch::multicore::getNumberOfPendingTasks();

          if ( numberOfPendingTasks>0 ) {
            std::pair<int,int> fusion = strategy->getNumberOfTasksToFuseAndTargetDevice();

            if (fusion.first>0 and numberOfPendingTasks>=fusion.first) {
              tarch::multicore::internal::fusePendingTasks(fusion);
            }
            else {
              int numberOfTasksToProcess = std::max(1,numberOfPendingTasks/2);
              tarch::multicore::processPendingTasks( numberOfTasksToProcess );
            }
          }
          else {
            #pragma omp taskyield
            #if defined(Parallel)
            // Allow MPI to make progress. We otherwise might starve MPI
            int flag;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, MPI_STATUS_IGNORE );
            #endif
          }
        }

	//std::cout << "thread " << i << " will now terminate" << std::endl;
      }
    }
    #pragma omp taskwait
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  #pragma omp atomic
  nonblockingTasksCounter++;

  #pragma omp task priority(BackgroundConsumerPriority) untied
  {
    while (job->run()) {
      #pragma omp taskyield
    }
    delete job;
    #pragma omp atomic
    nonblockingTasksCounter--;
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
  const std::vector< Task* >&  tasks,
  orchestration::Strategy*     strategy
) {
  if (strategy->processPendingTasksWhileWaitingInBSPSection()) {
    spawnAndWaitAsExplicitTasksWithPolling(tasks,strategy);
  }
  else {
    spawnAndWaitAsTaskLoop(tasks);
  }
}


void tarch::multicore::native::yield() {
  #pragma omp taskyield
}


int tarch::multicore::native::getNumberOfPendingTasks() {
  return nonblockingTasksCounter;
}


#endif
