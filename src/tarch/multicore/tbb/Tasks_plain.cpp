#include "tarch/Assertions.h"


#include <thread>

#include "tarch/multicore/multicore.h"
#include "tarch/logging/Statistics.h"


#if defined(SharedTBB) and !defined(LayeredMultitaskingRuntime)


#include "../Tasks.h"
#include "TBB.h"

#include "tarch/logging/Log.h"
#include "tarch/Assertions.h"


#include <vector>
#include <limits>
#include <cmath>


#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/concurrent_priority_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/parallel_for.h>


namespace {
  //
  // This is a bug in Intel's TBB as documented on
  //
  // https://software.intel.com/en-us/forums/intel-threading-building-blocks/topic/700057
  //
  // These task groups have to be static inside a cpp file.
  //
  static tbb::task_group_context  backgroundTaskContext(tbb::task_group_context::isolated);

  tbb::atomic<int> numberOfBackgroundTasks;

  /**
   * I have my own task class in Peano 4
   *
   * This class maps this Peano 4 class onto a plain TBB task. Peano's
   * tasks have a run() method and can reschedule.
   *
   * @todo Peano 4 tasks also have a priority which is not yet mapped
   *   onto a TBB task.
   */
  class TBBWrapperAroundTask: public tbb::task {
    private:
      tarch::multicore::Task*  _task;
    public:
      TBBWrapperAroundTask( tarch::multicore::Task* task ):
        _task(task) {
    	numberOfBackgroundTasks++;
      }

      tbb::task* execute() {
        bool reschedule = _task->run();
        if (reschedule) {
          ::tbb::task::enqueue(*new (::tbb::task::allocate_root(::backgroundTaskContext)) TBBWrapperAroundTask(_task) );
        }
        else {
          delete _task;
        }
        numberOfBackgroundTasks--;
        return nullptr;
      }
  };
}


/**
 * This routine should process up to maxTasks background tasks. However, this
 * does not work with plain TBB to the best of my knowledge, so I simply yield.
 */
bool tarch::multicore::processPendingTasks( int maxTasks ) {
  int oldNumber = numberOfBackgroundTasks;
  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, numberOfBackgroundTasks );
  yield();
  return numberOfBackgroundTasks < oldNumber;
}


/**
 * Used to be in the tbb namespace but now moved into std as C++ picks up on
 * multithreading, too
 */
void tarch::multicore::yield() {
  std::this_thread::yield();
}


/**
 * Spawns the task
 */
void tarch::multicore::spawnTask(Task*  task) {
  ::tbb::task::enqueue(*new (::tbb::task::allocate_root(::backgroundTaskContext)) TBBWrapperAroundTask(task) );

  ::backgroundTaskContext.set_priority(::tbb::priority_t::priority_low);
  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}


/**
 * I found the task_group to yield way better performance than the parallel for.
 *
 * In theory, a task could ask to be re-executed. Never happens here though.
 * Nevertheless, I use a while loop.
 */
void tarch::multicore::spawnAndWait(
  const std::vector< tarch::multicore::Task* >&  tasks
) {
  ::tbb::task_group g;
  for (auto& p: tasks) {
    g.run([=]{
      tarch::multicore::Task* task = p;
      while ( task->run() ) {}
      delete task;
    });
  }
  g.wait();
/*
  ::tbb::parallel_for(
    ::tbb::blocked_range<int>(0,tasks.size()),
    [&](const ::tbb::blocked_range<int>& r) {
      for(int i=r.begin(); i!=r.end(); ++i) {
        tarch::multicore::Task* task = tasks[i];
        task->run();
        delete task;
      }
    }
  );
  */
}


int tarch::multicore::getNumberOfPendingTasks() {
  return numberOfBackgroundTasks;
}


void tarch::multicore::tbb::shutdownTaskEnvironment() {
  static tarch::logging::Log _log( "tarch::multicore::tbb" );
  logTraceInWith1Argument( "shutdownConsumerTasks()", numberOfBackgroundTasks.fetch_and_add(0) );

  backgroundTaskContext.cancel_group_execution();

  logInfo( "shutdownConsumerTasks()", "still " << numberOfBackgroundTasks << " consumer tasks alive. Cancel all " );

  logTraceOut( "shutdownConsumerTasks()" );
}


#endif
