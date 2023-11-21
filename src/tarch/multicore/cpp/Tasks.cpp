#include "tarch/multicore/Tasks.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"

#include "config.h"

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
  std::map< tarch::multicore::TaskNumber, std::future<void>* >  pendingTasksWithDependencies;
}

//#define OpenMPHighThroughputWaiting


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

    while (currentNumberOfLoopTasks>0) {
      if (processPendingTasks(1,false)) { // I use FILO to increase cache hit probability
        currentNumberOfLoopTasks = numberOfLoopTasks;
      }
      else {
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

  assertionEquals(numberOfLoopTasks,0);
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


void  tarch::multicore::native::spawnTask(Task* job, const TaskNumber&  in, const TaskNumber&  out) {
// @todo
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::spawnTask(Task* job) {
  std::future<void> dontWait = std::async(std::launch::async, [job]() -> void {
    while (job->run()) {
      tarch::multicore::Core::getInstance().yield();
    }
    delete job;
  });
}

void tarch::multicore::native::processFusedTask(
  Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device
) {
  std::list<tarch::multicore::Task*> copyOfTasksOfSameType = tasksOfSameType;
  std::future<void> dontWait = std::async(std::launch::async, [myTask, copyOfTasksOfSameType, device]() -> void {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask);
    } else {
      delete myTask;
    }
  });
}


void tarch::multicore::native::drainTaskQueues() {}


void tarch::multicore::native::spawnTaskWithDependencies(Task* job, const TaskNumber& inDependency, const TaskNumber& taskNumber) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::spawnTaskWithDependencies(Task* job, const std::vector<TaskNumber>& inDependencies, const TaskNumber& taskNumber) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::waitForTasks( const std::vector<TaskNumber>& inDependencies ) {}

void tarch::multicore::native::waitForTask( const TaskNumber& taskNumber ){}

void tarch::multicore::native::registerTask( const TaskNumber&  number ) {}


#endif
