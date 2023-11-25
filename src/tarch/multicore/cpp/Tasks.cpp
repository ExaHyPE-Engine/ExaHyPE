#include "tarch/multicore/Tasks.h"

#include "config.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
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
  tarch::logging::Log _log("tarch::multicore");

  /**
   * Task outcomes of tasks that are not yet submitted
   */
  std::map<tarch::multicore::TaskNumber, std::future<void>*>  registeredTasks;
  std::map<tarch::multicore::TaskNumber, std::future<void>*>  spawnedTasks;
}


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

// @todo Semaphoren fuer Tables

void tarch::multicore::native::spawnTask(
  Task* job, const TaskNumber& inDependency, const TaskNumber& taskNumber
) {
  logInfo( "spawnTaskWithDependencies()", inDependency.toString() << " -> " << taskNumber.toString() );
  std:set<TaskNumber> inDependencies;
  inDependencies.insert( inDependency );
  tarch::multicore::native::spawnTask(job, inDependencies, taskNumber );
}


void tarch::multicore::native::spawnTask(
  Task* job, const std::set<TaskNumber>& inDependencies, const TaskNumber& taskNumber
) {
  logInfo( "spawnTaskWithDependencies()", toString( inDependencies ) << " -> " << taskNumber.toString() );

  assertion1(
    taskNumber != tarch::multicore::TaskNumber::NoDependency,
    taskNumber.toString()
  );

  if ( spawnedTasks.count(taskNumber)>0 and spawnedTasks[taskNumber].second!=nullptr ) {
    logWarning( "spawnTaskWithDependencies(...)", "task " << taskNumber.toString() << " is already among pending tasks, i.e. noone ever waited for it although we now submit new task with this number. Wait before spawning. " );
    native::waitForTask(taskNumber);
  }

  if (spawnedTasks.count(taskNumber)==0) {
    spawnedTasks.insert( std::pair<TaskNumber, std::future<void>* >(taskNumber,nullptr) );
  }
  else {
    assertion1( spawnedTasks[taskNumber].second==nullptr, taskNumber.toString() );
  }

/*
  keine in dependencies?

  spawnedTasks.insert( std::pair< tarch::multicore::TaskNumber, std::future<void>* >(
    taskNumber,
    std::async(std::launch::async, [job, inDependency, taskNumber]() -> void {
      while (job->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete job;
    })
  ));

  if (registeredTasks.count(taskNumber)>0) {
    registeredTasks.erase(taskNumber);
  }
*/

}


void tarch::multicore::native::waitForTask(const TaskNumber& taskNumber) {
  logInfo( "waitForTasks()", "wait for task " << taskNumber.toString() );
  assertion1( registeredTasks.count(taskNumber)==0, taskNumber.toString() );
  assertion1( spawnedTasks.count(taskNumber)>0, taskNumber.toString() );
  assertion1( spawnedTasks.at(taskNumber).second!=nullptr, taskNumber.toString() );

  spawnedTasks[taskNumber].second->wait();
  delete spawnedTasks[taskNumber].second;
  spawnedTasks[taskNumber].second = nullptr;
}

void tarch::multicore::native::registerTask( const TaskNumber&  number ) {
  assertion( registeredTasks.count(number)==0 );
  logInfo( "waitForTasks()", "register task " << number.toString() << " (will be defined properly later)" );
  registeredTasks.insert(number);
}

void tarch::multicore::native::addDependency( const TaskNumber& taskNumber, const TaskNumber& registeredTasksNumber ) {
  logInfo( "waitForTasks()", "add dependency to future task: " << taskNumber.toString() << " -> " << registeredTasksNumber.toString() );
  assertion2( registeredTasks.count(taskNumber)==0, taskNumber.toString(), registeredTasksNumber.toString() );
  assertion2( registeredTasks.count(registeredTasksNumber)>0, taskNumber.toString(), registeredTasksNumber.toString() );
}

void tarch::multicore::native::waitForAllTasks() {
  logInfo( "waitForAllTasks()", "wait" );
}


// tarch::multicore::TaskNumber, std::future<void>*

#endif
