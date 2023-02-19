#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/sycl/Core.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"


#if defined(SharedSYCL)
#include <tbb/task_group.h>
#include <tbb/task_arena.h>

namespace {
  tarch::multicore::BooleanSemaphore  suspensionPointSemaphore;

  /**
   * Seems that there is no atomic in tbb. So I use the standard one.
   */
  std::atomic<int> nonblockingTasksCounter          = 0;

  /**
   * Native mapping of a task loop onto a SYCL loop
   *
   *
   * @see spawnTask()
   *
   */
  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    assertion( not tasks.empty() );

    for (auto& p: tasks) {
      tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
        while (p->run()) {}
        delete p;
      });
    }

    tarch::multicore::getHostSYCLQueue().wait();
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * This is a straightforward translation of the OpenMP high-throughput
   * realisation.
   *
   * @see spawnAndWaitAsTaskLoop() for realisation details re TBB/SYCL.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    std::atomic<int>       pendingTasks  = NumberOfThreads;
    std::atomic<int>       spawnedTasks  = 0;

    for (auto& p: tasks) {
      tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
        h.host_task([&]() {
          spawnedTasks++;

          while (p->run()) {}
          delete p;
  
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
              tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
              processedSomeTasks = tarch::multicore::internal::fusePendingTasks(fusion);
            }
            else if (numberOfPendingTasks>0) {
              tarch::multicore::internal::processThreadLocalTask(threadNumber);
              }
            else { // no local tasks left over
              tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
              const int numberOfTasksToProcess = std::max(1,tarch::multicore::getNumberOfPendingTasks(-1)/2);
              processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
            }
          }
        });
      });
    }

    // @todo This is not nice. I don't want to wait for all events. I would like
    // to wait for BSP events only
    tarch::multicore::getHostSYCLQueue().wait();
  }
}


/**
 * Read spawnAndWaitAsTaskLoop() first.
 *
 * ## Implementation details
 *
 * The tasks here are always spawned into the host queue. Consequently, they
 * will run on the CPU and thus should be able to invoke virtual functions and
 * access global variables. Our experiments with Intel DPC++ however suggest
 * that this is not the case and that SYCL/DPC++ refuses to translate this
 * code: you have to spawn explicitly as host task.
 *
 *
 * @author Andrew Mallinson
 * @author Tobias Weinzierl
 *
 *
 * @see spawnAndWaitAsTaskLoop()
 */
void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasksCounter++;

  sycl::event e = tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
    h.host_task([=]() {
      while (job->run()) {}
      delete job;
      nonblockingTasksCounter--;
    });
  });

  e.wait();
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
  tarch::multicore::getHostSYCLQueue().wait();
}


#endif
