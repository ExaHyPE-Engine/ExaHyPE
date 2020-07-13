#include "tarch/Assertions.h"


#include <thread>
#include <queue>
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Statistics.h"


#ifdef SharedTBB

#if !defined(noTBBPrefetchesJobData) and !defined(TBBPrefetchesJobData)
  #define TBBPrefetchesJobData
#endif

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

  typedef tbb::concurrent_priority_queue< tarch::multicore::Task* >   TaskQueue;

  TaskQueue nonblockingTasks;

  tbb::atomic<int> numberOfConsumerTasks;

  /**
   * This is a task which consumes background jobs, as it invokes
   * processBackgroundJobs(). Typically, I make such a job consume up to
   * half of the available background jobs, before it then stops the
   * processing. When it stops and finds out that there would still
   * have been more jobs to process, then it enqueues another consumer task
   * to continue to work on the jobs at a later point.
   */
  class ConsumerTask: public tbb::task {
    private:
      const int _maxJobs;

      ConsumerTask(int maxJobs):
        _maxJobs( std::max(1,maxJobs) ) {
        ::tarch::logging::Statistics::getInstance().log( ConsumerTaskCountStatisticsIdentifier,   numberOfConsumerTasks, true );
        ::tarch::logging::Statistics::getInstance().log( TasksPerConsumerRunStatisticsIdentifier, _maxJobs, true );
      }

    public:
      /**
       * Schedule a new background job consumer task. We have to tell
       * each consumer how many jobs it may process at most. The tasks
       * have to be tied to backgroundTaskContext to remain valid.
       */
      static void enqueue(int maxTasks = nonblockingTasks.size()) {
        numberOfConsumerTasks.fetch_and_add(1);
        ConsumerTask* tbbTask = new (tbb::task::allocate_root(::backgroundTaskContext)) ConsumerTask(maxTasks);
        tbb::task::enqueue(*tbbTask);
        //::backgroundTaskContext.set_priority(tbb::priority_t::priority_low);
        //::backgroundTaskContext.set_priority(tbb::priority_t::priority_high);
      }

      ConsumerTask(const ConsumerTask& copy):
        _maxJobs(copy._maxJobs) {
      }

      /**
       * Process _maxJobs from the background job queue
       *
       * If there had been tasks to be processed, then we know that
       * processPendingTasks() would have spawned a new consumer. So we
       * are all fine. If we have not got any work to do, then we might
       * terminate. However, this would be stupid as the whole idea of
       * background tasks is that they automatically are processed
       * whenever the code tends to become idle. So if we haven't got
       * any data, then we reschedule ourselves but with a lower job
       * count. Unless we have already rescheduled ourselves tons of
       * times. In this case, we quit.
       *
       * @see processPendingTasks()
       * @see enqueue()
       */
      tbb::task* execute() {
        std::cout << "[[[[[ " << _maxJobs << " ]]]]]";
        bool processedJob = tarch::multicore::processPendingTasks(_maxJobs);

        ::tarch::logging::Statistics::getInstance().log( ConsumerTaskCountStatisticsIdentifier,   numberOfConsumerTasks );
        ::tarch::logging::Statistics::getInstance().log( TasksPerConsumerRunStatisticsIdentifier, _maxJobs );

        numberOfConsumerTasks.fetch_and_add(-1);

        return nullptr;
      }
  };


  class TBBWrapperAroundTask: public tbb::task {
    private:
      tarch::multicore::Task*  _task;
    public:
      TBBWrapperAroundTask( tarch::multicore::Task* task ):
        _task(task) {
      }

      tbb::task* execute() {
        while ( _task->run() ) {}
        delete _task;
        return nullptr;
      }
  };
}


/**
 * Process up to maxTasks from the background queue
 *
 * <h2> Task forking </h2>
 *
 * If we have processed a task, I spawn a new consumer task before I return.
 * The rationale is simple: This routine is either called by the user code as
 * it waits for some results from a background job, or it is called by a
 * consumer task. In the first case, we have just entered a phase in the
 * calling thread where we now need results from tasks that are otherwise not
 * that critical. We may therefore assume that we entered a phase where we are
 * starting to reduce the concurrency. It is thus reasonable to span a new
 * consumer task to help out. If we are called by an existing consumer task
 * and there had been work to do, then it would be unreasonable not to
 * continue to process work. Therefore, we span another consumer, but this
 * time with a slightly bigger number of tasks. After all, we wanna get
 * stuff out of the door.
 *
 * So once a consumer task becomes alive, it continues to stay (as it
 * effectively reschedules itself) until it hits an empty task queue. If the
 * user code polls the task outcomes and processes one (or more) tasks
 * in-between, then it effectively serves as spawn mechanism for tons of
 * consumer tasks.
 *
 * @return Have processed at least one task
 */
bool tarch::multicore::processPendingTasks( int maxTasks ) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier,        tarch::multicore::getNumberOfPendingTasks() );

  bool  result         = false;
  Task* myTask         = nullptr;
  Task* myPrefetchTask = nullptr;
  bool  spawnConsumer  = maxTasks==0;

  const int   backupOfMaxTasks = maxTasks;

  while ( maxTasks>0 or myPrefetchTask!=nullptr) {
    bool gotTask = false;
    if (myPrefetchTask==nullptr) {
      gotTask = nonblockingTasks.try_pop(myTask);
    }
    else {
      gotTask = true;
      myTask = myPrefetchTask;
    }

    #if defined(TBBPrefetchesJobData)
    bool gotPrefetchTask = nonblockingTasks.try_pop(myPrefetchTask);
    if (gotPrefetchTask) {
      myPrefetchTask->prefetch();
    }
    else {
      myPrefetchTask = nullptr;
    }
    #endif

    if (gotTask) {
      bool requeue = myTask->run();
      if (requeue) {
        spawnTask( myTask );
      }
      else {
        delete myTask;
      }
      maxTasks--;
      result = true;
      spawnConsumer = true;
    }
    else {
      maxTasks=0;
    }
  }

  if (spawnConsumer) {
    ConsumerTask::enqueue( backupOfMaxTasks+1 );
  }

  return result;
}


/**
 * Used to be in the tbb namespace but now moved into std as C++ picks up on
 * multithreading, too
 */
void tarch::multicore::yield() {
  std::this_thread::yield();
}


/**
 * Spawns the task, i.e. puts is into the queue of pending tasks.
 *
 * In my original code, I made the spawnTask() routine ensure that
 * there's always at least one consumer up and running. Spawning
 * tasks however is expensive, and it is important that this
 * routine returns asap - after all, it is used to spawn a background
 * task which has low priority and should not delay the main thread.
 * Therefore, I removed the auto-spawn. Background tasks now are
 * parked in the background task queue, i.e. we assume that Peano is
 * capable of using all cores while it spawns these background tasks.
 * It is then up to the actual task processing to decide whether it
 * is reasonable to spawn a few additional tasks to help to tidy
 * things up.
 */
void tarch::multicore::spawnTask(Task*  task) {
  nonblockingTasks.push( task );

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}


/**
 * I found the task_group to yield way better performance than the parallel for.
 * However, it seems that the task_group vetoes other consumers to become active
 * even if it does not require all cores. This is a pity but nothing I can solve
 * right now.
 *
 * @todo Escalate via Intel ticket
 */
void tarch::multicore::spawnAndWait(
  const std::vector< tarch::multicore::Task* >&  tasks
) {
  ::tbb::task_group g;
  for (auto& p: tasks) {
    g.run([=]{
      tarch::multicore::Task* task = p;
      task->run();
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
  return nonblockingTasks.size();
}


void tarch::multicore::tbb::shutdownConsumerTasks() {
  static tarch::logging::Log _log( "tarch::multicore::tbb" );
  logTraceInWith1Argument( "shutdownConsumerTasks()", numberOfConsumerTasks.fetch_and_add(0) );

  backgroundTaskContext.cancel_group_execution();

  logInfo( "shutdownConsumerTasks()", "still " << numberOfConsumerTasks << " consumer tasks alive. Cancel all " );

  logTraceOut( "shutdownConsumerTasks()" );
}


#endif
