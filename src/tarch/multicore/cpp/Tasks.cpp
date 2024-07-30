#include "tarch/multicore/Tasks.h"

#include "config.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"

#if defined(GPUOffloadingCPP)
#include <algorithm>
#include <execution>
#include <iterator>
#include <numeric>
#include <vector>
#endif

#if defined(SharedCPP)

#include <atomic>
#include <execution>
#include <future>


namespace {
  /**
   * Logging
   */
  tarch::logging::Log _log("tarch::multicore::cpp");

  /**
   * Keep track of fact if any spawned task has terminated.
   */
  std::map<tarch::multicore::TaskNumber, std::shared_future<void>*> spawnedTasks;
  tarch::multicore::BooleanSemaphore                                spawnedTasksSemaphore;
} // namespace


// #define OpenMPHighThroughputWaiting


#if defined(OpenMPHighThroughputWaiting)
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  std::atomic<int> numberOfLoopTasks = tasks.size(); // bookkeeping of how many loop tasks
                                                     // we have in the system

  std::for_each(std::execution::par_unseq, tasks.begin(), tasks.end(), [](auto&& item) -> void {
    while (tasks[i]->run()) {
      tarch::multicore::Core::getInstance().yield();
    }
    delete tasks[i];

    int currentNumberOfLoopTasks;                   // reduce the bookkeeping
    currentNumberOfLoopTasks = --numberOfLoopTasks; // own loop termination
                                                    // criterion

    while (currentNumberOfLoopTasks > 0) {
      if (processPendingTasks(1, false)) { // I use FILO to increase cache hit probability
        currentNumberOfLoopTasks = numberOfLoopTasks;
      } else {
        currentNumberOfLoopTasks = 0; // This is not nice. It means that a
                                      // fast task might finish while ready
                                      // tasks literally become a available a
                                      // second later. However, without his
                                      // statement, another loop task might
                                      // never be able to run, as the busy
                                      // polling keeps the thread idle. We
                                      // starve.
      }
    }
  });

  assertionEquals(numberOfLoopTasks, 0);
}
#else
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  std::for_each(std::execution::par_unseq, tasks.begin(), tasks.end(), [](auto&& item) -> void {
    while (item->run()) {
      std::this_thread::yield();
    }
    delete item;
  });
}
#endif


void tarch::multicore::native::spawnTask(
  Task* job, const std::set<TaskNumber>& inDependencies, const TaskNumber& taskNumber) {
  {
    tarch::multicore::Lock lock(spawnedTasksSemaphore);

    // is_ready() will enter the standard soon according to change requests
    // if ( spawnedTasks.count(taskNumber)>0 and not spawnedTasks.at(taskNumber)->is_ready()) {
    if (spawnedTasks.count(taskNumber) > 0 and spawnedTasks.at(taskNumber)->wait_until(std::chrono::system_clock::time_point::min()) != std::future_status::ready) {
      logWarning(
        "spawnTaskWithDependencies(...)",
        "task " << taskNumber << " is already among submitted tasks, i.e. noone ever waited for it although we now submit new task with this number. Wait before spawning. "
      );
      lock.free(); // if task recursively locks table
      waitForTask(taskNumber);
    }
  }

  logInfo("spawnTask(...)", "submit task " << taskNumber);
  std::shared_future<void> submittedTask = std::async(std::launch::async, [job, taskNumber, inDependencies]() -> void {
    logInfo("spawnTask(...)", "kick off task " << taskNumber);
    tarch::multicore::native::waitForTasks(inDependencies);
    while (job->run()) {
      tarch::multicore::Core::getInstance().yield();
    }
    delete job;
  });

  if (taskNumber != NoOutDependencies) {
    tarch::multicore::Lock lock(spawnedTasksSemaphore);
    logInfo("spawnTask(...)", "store shared future of task " << taskNumber);
    assertion1(spawnedTasks.count(taskNumber) == 0, taskNumber);
    spawnedTasks[taskNumber] = new std::shared_future<void>(submittedTask);
  }
}


void tarch::multicore::native::processFusedTask(Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device) {
  std::list<tarch::multicore::Task*> copyOfTasksOfSameType = tasksOfSameType;
  std::future<void>                  dontWait              = std::async(std::launch::async, [myTask, copyOfTasksOfSameType, device]() -> void {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
    } else {
      delete myTask;
    }
  });
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  for (auto& p : inDependencies) {
    std::shared_future<void>* checkFuture = nullptr;

    tarch::multicore::Lock lock(spawnedTasksSemaphore);
    // check if there's something
    // @todo
    if (spawnedTasks.count(p) > 0 and spawnedTasks.at(p)->valid()) {
      checkFuture = spawnedTasks.at(p);
    }
    lock.free();

    // check but let someone else later tidy it up
    // @todo intod ocu
    if (checkFuture != nullptr) {
      logInfo("waitForTasks(...)", "wait for task " << p);
      checkFuture->wait();
    }

    // tidy up. Could happen that it is eliminated by now so better doublecheck
    lock.lock();
    if (spawnedTasks.count(p) > 0) {
      delete spawnedTasks.at(p);
      spawnedTasks.erase(p);
    }
    lock.free();
  }
}


void tarch::multicore::native::waitForAllTasks() {
  logTraceInWith1Argument("waitForAllTasks()", spawnedTasks.size());
  int firstUnfinishedTask = NoOutDependencies + 1;
  while (firstUnfinishedTask != NoOutDependencies) {
    firstUnfinishedTask = NoOutDependencies;

    tarch::multicore::Lock lock(spawnedTasksSemaphore);
    if (not spawnedTasks.empty()) {
      firstUnfinishedTask = spawnedTasks.begin()->first;
    }
    lock.free();

    if (firstUnfinishedTask != NoOutDependencies) {
      logInfo("waitForAllTasks()", "wait for task " << firstUnfinishedTask);
      waitForTask(firstUnfinishedTask);
    }
  }
  logTraceOut("waitForAllTasks()");
}


#endif
