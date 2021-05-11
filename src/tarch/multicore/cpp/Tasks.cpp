#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"

#if defined(SharedCPP)

#include <algorithm>
#include <execution>
#include <future>

namespace {
  std::atomic<int> nonblockingTasks                 = 0;

  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    std::for_each(
      std::execution::par_unseq,
      tasks.begin(),
      tasks.end(),
      [](auto&& task) {
        while (task->run()) {}
        delete task;
      }
    );
  }


  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    assertion( not tasks.empty() );
    assertion( tarch::multicore::getRealisation()!=tarch::multicore::Realisation::MapOntoNativeTasks );

    std::atomic<int> busyThreads = static_cast<int>(tasks.size());

    std::for_each(
      std::execution::par_unseq,
      tasks.begin(),
      tasks.end(),
      [&busyThreads](auto&& task) {
        while (task->run()) {}
        delete task;

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
          tarch::multicore::processPendingTasks( std::max(1,tarch::multicore::getNumberOfPendingTasks()/2) );
        }
      }
    );
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasks++;

  std::thread t([job]()->void {
    while (job->run()) {}
    delete job;
    nonblockingTasks--;
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
  std::this_thread::yield();
}


int tarch::multicore::native::getNumberOfPendingTasks() {
  return nonblockingTasks;
}


#endif
