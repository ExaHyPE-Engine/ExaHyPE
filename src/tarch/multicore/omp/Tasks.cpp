#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/Statistics.h"


#if defined(SharedOMP)

namespace {
  int nonblockingTasksCounter          = 0;

  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;

  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");
  // @todo nachziehen in anderen implementierungen
  const std::string BackfillWhileWaitingForBSPTasks( "tarch::multicore::backfill");


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
    }
    #pragma omp taskwait // wait for all elements from tasks to complete
                         // do not wait for the children of tasks
                         // but do children if you have time (this does not
                         // happen however)
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * This implementation variant follows the patterns described in the 2021
   * IWOMP paper by H. Schulz et al, but it is way more advanced. An important
   * paper to read in the context of the present solution is also
   * https://www.osti.gov/pages/servlets/purl/1465188. Note in particular the
   * example in Figure 3.
   *
   * # Realisation
   *
   * The idea is that we spawn one task per bsp segment (stored within tasks).
   * If a task terminates, we do not terminate immediately but first check if
   * all the other tasks have already finished, too. If they haven't, we
   * process a few pending tasks from the queue and then check again. We
   * prioritise throughput over an immediate termination of the BSP section.
   *
   * As we might have fewer tasks or more tasks than cores, we take the maximum
   * of both of them. If the a task is spawned that does not correspond to an
   * entry in tasks, i.e. if tasks has fewer entries than threads, this task
   * will not process a BSP task but immediately start to run through the
   * pending tasks.
   *
   * The devil is in the details when it comes to the actual while loop that
   * processes pending tasks:
   *
   * - The while loop should terminate once all tasks of the BSP section have
   *   done their core work. We introduce a shared atomic variable pendingTasks
   *   which counts down. Whenever a BSP task has finished its core work, it
   *   decrements it. The while loop continues if pendingTasks reduces to zero.
   * - If we have more tasks than threads, we should not allow a BSP task to
   *   process pending tasks. It might become so busy with processing these guys
   *   that it actually starves other BSP-type tasks.
   * - We really do not know in OpenMP when a ready task is processed. The task
   *   loop might trigger all tasks, do them, and then wait. This is the kind
   *   pattern that we want to have. However, it could also be that two tasks
   *   start, the machine then does busy waiting in the waitall and the other
   *   tasks do not start. Therefore, it is important that we introduce a second
   *   atomic counter which counts how many tasks have actually kicked off. If
   *   and only if all tasks that are ready had a chance to run, we know that
   *   all BSP stuff is now in work. In this case, we can also enable the
   *   polling. But not before.
   *
   * # System-specific details
   *
   * NVIDIA's nvcc does not support the taskloop. It would be easy to replace
   * the taskloop with a for loop and tasks within the loop body. But I decided
   * to strive for a clean code.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    int       pendingTasks  = NumberOfThreads;
    int       spawnedTasks  = 0;

    #pragma omp taskloop nogroup shared(pendingTasks, spawnedTasks) priority(StandardPriority) untied
    for (int i=0; i<NumberOfThreads; i++) {
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

        const auto fusion       = strategy->getNumberOfTasksToFuseAndTargetDevice();
        const int  threadNumber = tarch::multicore::Core::getInstance().getThreadNumber();


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
          tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
          int numberOfPendingTasks = tarch::multicore::getNumberOfPendingTasks(threadNumber);

          bool processedSomeTasks = false;
          if (numberOfPendingTasks>=fusion.minTasks) {
            processedSomeTasks = tarch::multicore::internal::fusePendingTasks(fusion);
          }
          else if (numberOfPendingTasks>0) {
            tarch::multicore::internal::processThreadLocalTasks(threadNumber);
          }
          else { // no local tasks left over
            tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
            const int numberOfTasksToProcess = std::max(1,tarch::multicore::getNumberOfPendingTasks(-1)/2);
            ::tarch::logging::Statistics::getInstance().log( BackfillWhileWaitingForBSPTasks, numberOfTasksToProcess );
            processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
          }
        }
    }
    #pragma omp taskwait // wait for all elements from tasks to complete
                         // do not wait for the children of tasks
                         // but do children if you have time (this does not
                         // happen however)
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


int tarch::multicore::native::getNumberOfPendingTasks() {
  return nonblockingTasksCounter;
}


/**
 * It is not clear what happens if I use tasksOfSameType directly within the task.
 * It is clear that a pure reference cannot be used within a task, as the task uses
 * firstprivate by default. Therefore, it would copy something which ceases to exist.
 * It is not clear what happens with const references, but we had some memory issues
 * with the newer Intel/LLVM versions. Therefore, I manually copy the list and pass
 * this one into fuse(). The firstprivate annotation is not required, as this is the
 * default. But I leave it in here to highlight that three variables are copied,
 * but tasksOfSameType is not copied and not used within the task.
 */
void tarch::multicore::native::processFusedTask(Task*  myTask, const std::list< tarch::multicore::Task* >& tasksOfSameType, int device) {
  std::list< tarch::multicore::Task* > copyOfTasksOfSameType = tasksOfSameType;
  #pragma omp task firstprivate(myTask,copyOfTasksOfSameType,device)
  {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType,device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask);
    }
    else {
      delete myTask;
    }
  }
}


void tarch::multicore::native::deferRunningThreadAndContinueWithOtherTasks() {
  #pragma omp taskyield
}

#endif
