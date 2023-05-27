#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"

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

  const std::string BSPConcurrencyLevelStatisticsIdentifier( "tarch::multicore::bsp-concurrency-level" );

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
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, 1.0, true );
        #endif
        while (item->run()) {
          #if !defined(GPUOffloadingCPP)
            std::this_thread::yield();
          #endif
        }
        delete item;
        #if !defined(GPUOffloadingCPP)
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -1.0, true );
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
        ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, 1.0, true );
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
        ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -1.0, true );
      #endif

      pendingTasks--;

      bool continueToProcessPendingTasks =
        pendingTasks>0
        and
        pendingTasks<tarch::multicore::Core::getInstance().getNumberOfThreads()
        and
        spawnedTasks==NumberOfThreads;
      while ( continueToProcessPendingTasks ) {
        continueToProcessPendingTasks = ::tarch::multicore::internal::progressTasksWithinSpawnAndWaitAsExplicitTasksWithPolling(
          strategy
        );
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


#endif
