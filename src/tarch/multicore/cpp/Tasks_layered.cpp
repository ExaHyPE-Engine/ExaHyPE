#include "tarch/Assertions.h"
#include "../Tasks.h"
#include "tarch/multicore/multicore.h"


#ifdef SharedCPP
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>

#include "CPP.h"
#include "tarch/logging/Log.h"


/**
 * This class is a 1:1 mirror of my TBB implementation. Please see
 * documentation there.
 */
namespace {
  std::queue<tarch::multicore::Task* > nonblockingTasks;
  std::mutex                           taskQueueMutex;
}



void tarch::multicore::yield() {
  std::this_thread::yield();
}


bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);
  bool result = false;
  while ( maxTasks>0 ) {
	// try to get a task and store in p
    Task* p = nullptr;
    taskQueueMutex.lock();
    if (not nonblockingTasks.empty()) {
      p = nonblockingTasks.front();
      nonblockingTasks.pop();
    }
    taskQueueMutex.unlock();

    // process task
    if (p!=nullptr) {
      bool requeue = p->run();
      if (requeue) {
        spawnTask( p );
      }
      else {
        delete p;
      }
      maxTasks--;
      result = true;
    }
    else {
      maxTasks=0;
    }
  }
  return result;
}


namespace {
  std::atomic<int> numberOfConsumerTasks(0);


  void spawnConsumerTask( int numberOfTasks = tarch::multicore::getNumberOfPendingTasks() ) {
    numberOfConsumerTasks++;

    std::thread([numberOfTasks]() {
      bool hasProcessedTasks = tarch::multicore::processPendingTasks( numberOfTasks );
      int  newNumberOfTasks  = tarch::multicore::getNumberOfPendingTasks();

      if (hasProcessedTasks and newNumberOfTasks>numberOfTasks*2) {
        spawnConsumerTask( numberOfTasks );
        spawnConsumerTask( numberOfTasks );
      }
      else if (hasProcessedTasks and newNumberOfTasks>numberOfTasks) {
        spawnConsumerTask( numberOfTasks+1 );
      }
      else if (hasProcessedTasks and numberOfTasks>1) {
        spawnConsumerTask( numberOfTasks-1 );
      }
      numberOfConsumerTasks--;
    }).detach();
  }
}


void tarch::multicore::spawnTask(Task*  job) {
  taskQueueMutex.lock();
  nonblockingTasks.push(job);
  taskQueueMutex.unlock();

  if (numberOfConsumerTasks==0) {
    spawnConsumerTask();
  }
}


void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  std::thread t[ tasks.size() ];

  for (int i = 0; i < tasks.size(); ++i) {
	t[i] = std::thread( [&,i]()->void {
      while (tasks[i]->run()) {}
      delete tasks[i];
	});
  }

  for (int i = 0; i < tasks.size(); ++i) {
	t[i].join();
  }
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}


void tarch::multicore::cpp::shutdownConsumerTasks() {
  static tarch::logging::Log _log( "tarch::multicore::cpp" );
  logTraceInWith1Argument( "shutdownConsumerTasks()", numberOfConsumerTasks.fetch_add(0) );
  while (numberOfConsumerTasks.fetch_add(0)>0) {
	yield();
  }
  logTraceOut( "shutdownConsumerTasks()" );
}


#endif
