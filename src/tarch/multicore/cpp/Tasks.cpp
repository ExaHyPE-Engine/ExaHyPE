#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/Statistics.h"

#if defined(GPUOffloadingCPP)

#include <vector>
#include <iterator>
#include <execution>
#include <algorithm>
#include <numeric>

#endif

#if defined(SharedCPP)

#include <atomic>
#include <execution>
#include <future>

namespace {
  std::atomic<int> nonblockingTasksCounter          = 0;

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

    std::for_each(
      std::execution::par_unseq,
      tasks.begin(),
      tasks.end(),
      [](auto&& item) -> void {
        #if !defined(GPUOffloadingCPP)
        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
        #endif
        while (item->run()) {
          #if !defined(GPUOffloadingCPP)
          std::this_thread::yield();
          #endif
        }
        delete item;
        #if !defined(GPUOffloadingCPP)
        ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
        #endif
      }
    );
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * Please see the OpenMP discussion for details.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    std::atomic<int>       pendingTasks  = NumberOfThreads;
    std::atomic<int>       spawnedTasks  = 0;

    //&spawnedTasks, &pendingTasks//
    auto bspLoopBody = [&](int iterationCounter) -> void {
      spawnedTasks++;

      #if !defined(GPUOffloadingCPP)
      ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
      #endif
      if (iterationCounter<tasks.size()) {
        while (tasks[iterationCounter]->run()) {
          #if !defined(GPUOffloadingCPP)
          std::this_thread::yield();
          #endif
        }
        delete tasks[iterationCounter];
      }

      #if !defined(GPUOffloadingCPP)
      ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
      #endif

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
          internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
          processedSomeTasks = tarch::multicore::internal::fusePendingTasks(fusion);
        }
        else if (numberOfPendingTasks>0) {
          tarch::multicore::internal::processThreadLocalTask(threadNumber);
        }
        else { // no local tasks left over
          tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
          const int numberOfTasksToProcess = std::max(1,tarch::multicore::getNumberOfPendingTasks(-1)/2);
          #if !defined(GPUOffloadingCPP)
          ::tarch::logging::Statistics::getInstance().log( BackfillWhileWaitingForBSPTasks, numberOfTasksToProcess );
          #endif
          processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
        }
      }
    };


    std::vector< std::thread* > threads;

    for (int i=0; i<NumberOfThreads; i++) {
      std::thread* newThread = new std::thread(bspLoopBody,i);
      threads.push_back( newThread );
    }

    for (auto& p: threads) {
      p->join();
    }
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasksCounter++;

  std::future<void> dontWait = std::async( std::launch::async,
    [job] () -> void {
      while (job->run()) {
        #if !defined(GPUOffloadingCPP)
        std::this_thread::yield();
        #endif
      }
      delete job;
    }
  );

  nonblockingTasksCounter--;
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
  std::future<void> dontWait = std::async( std::launch::async,
    [myTask,copyOfTasksOfSameType,device]() -> void {
      bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType,device);
      if (stillExecuteLocally) {
        tarch::multicore::native::spawnTask(myTask);
      }
      else {
        delete myTask;
      }
    }
  );
}


void tarch::multicore::native::deferRunningThreadAndContinueWithOtherTasks() {
  #if !defined(GPUOffloadingCPP)
  std::this_thread::yield();
  #endif
}

#endif
