#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/Statistics.h"


#if defined(SharedSYCL)

#include <CL/sycl.hpp>
#include <tbb/task_group.h>
#include <tbb/task_arena.h>

namespace {
  tarch::multicore::BooleanSemaphore  suspensionPointSemaphore;

  /**
   * Seems that there is no atomic in tbb. So I use the standard one.
   */
  std::atomic<int> nonblockingTasksCounter          = 0;

  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");

  oneapi::tbb::task_group backgroundTaskGroup;

  /**
   * This queue should be on the CPU and have a rather low priority.
   * Default of queues is out-of-order, so we are fine here.
   *
   * You can't use a queue here, as our tasks have side-effects and use
   * virtual function calls.
   */
  //sycl::queue backgroundTaskQueue(sycl::cpu_selector());
  //sycl::queue backgroundTaskQueue;

  /**
   * Native mapping of a task loop onto a SYCL/oneTBB loop
   *
   * I didn't really understand yet whether SYCL has the concept of a task loop or a
   * parallel loop outside of queues, but if we hit this routine, we don't want to submit
   * something to a queue. Instead, we want to execute stuff right away. Therefore, I
   * rely on oneTBB's task group, assuming that it behaves similar to OpenMP's parallel
   * for.
   */
  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    // important, otherwise taskwait synchronises
    assertion( not tasks.empty() );

    // is implicitly executed within root arena, as the invoking task is
    // run within root arena
    oneapi::tbb::task_group taskGroup;
    for (auto& p: tasks) {
      taskGroup.run(
        [&tasks,p]() -> void {
          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
          while (p->run()) {}
          delete p;
          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
        }
      );
    }
    taskGroup.wait();
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * This is a straightforward translation of the OpenMP high-throughput
   * realisation.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    std::atomic<int>       pendingTasks  = NumberOfThreads;
    std::atomic<int>       spawnedTasks  = 0;

    oneapi::tbb::task_group taskGroup;
    for (int i=0; i<NumberOfThreads; i++) {
      // is implicitly executed within root arena
      taskGroup.run(
        [&spawnedTasks,&pendingTasks,&tasks,i,NumberOfThreads,&strategy] () -> void {
          spawnedTasks++;

          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
          if (i<tasks.size()) {
            while (tasks[i]->run()) {}
            delete tasks[i];
          }
          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
  
          pendingTasks--;

          const std::pair<int,int> fusion = strategy->getNumberOfTasksToFuseAndTargetDevice();

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
            int numberOfPendingTasks = tarch::multicore::getNumberOfPendingTasks();

            bool processedSomeTasks = false;
            if (fusion.first>0 and numberOfPendingTasks>=fusion.first) {
              processedSomeTasks = tarch::multicore::internal::fusePendingTasks(fusion);
            }
            else {
              int numberOfTasksToProcess = std::max(1,numberOfPendingTasks/2);
              processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
            }

            if (not processedSomeTasks) {
              tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
            }
          }
        }
      );
    }
    taskGroup.wait();
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasksCounter++;

  // Does not work, as we have virtual functions, too.
/*
  backgroundTaskQueue.submit([&] (sycl::handler& commandGroupHandler) {
    commandGroupHandler.single_task(
      [job] () -> void {
        //nonblockingTasksCounter--;
      }
    );
  });
*/


  // Serial version
/*
  while (job->run()) {}
  delete job;
  nonblockingTasksCounter--;
*/

  backgroundTaskGroup.run([&,job]{
    while (job->run()) {}
    delete job;
    nonblockingTasksCounter--;
  });
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
   bool stillExecuteLocally = myTask->fuse(tasksOfSameType,device);
   if (stillExecuteLocally) {
     tarch::multicore::native::spawnTask(myTask);
   }
   else {
     delete myTask;
   }
}


void tarch::multicore::native::deferRunningThreadAndContinueWithOtherTasks() {
  backgroundTaskGroup.wait();
}


#endif
