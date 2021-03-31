#include "tarch/Assertions.h"
#include "../Tasks.h"
#include "../Core.h"
#include "tarch/multicore/multicore.h"

#if defined(SharedOMP) and defined(LayeredMultitaskingRuntime)

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <list>
#include <cstdlib>
#include "../Tasks.h"

#include "tarch/logging/Statistics.h"


namespace {
  std::list< tarch::multicore::Task* > nonblockingTasks;
  std::mutex                           taskQueueMutex;
  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;

  /**
   * Determines how to handle the tasks dumped into nonblockingTasks.
   *
   * @see tarch::multicore::spawnAndWait()
   */
  enum class TaskProgressionStrategy {
    BufferInQueue,
    MapOntoOMPTask,
    MergeTasks
  };

  /**
   * This flag/strategy is toggled by tarch::multicore::spawnAndWait(). The
   * flag determines how
   * tarch::multicore::processPendingTasks(int) processes the tasks, and it
   * also controls how spawnTask() works.
   *
   * @see tarch::multicore::spawnAndWait()
   * @see tarch::multicore::processPendingTasks(int)
   */
  TaskProgressionStrategy  taskProgressionStrategy = TaskProgressionStrategy::MapOntoOMPTask;

  const std::string MapPendingTasksOnOpenMPTasksStatisticsIdentifier( "tarch::multicore::map-pending-tasks-onto-openmp-tasks");
  const std::string MergeTasksStatisticsIdentifier( "tarch::multicore::merge-tasks");


  bool processOnePendingTaskLIFO() {
    tarch::multicore::Task* myTask = nullptr;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (not nonblockingTasks.empty()) {
        myTask = nonblockingTasks.back();
        nonblockingTasks.pop_back();
      }
    }
    if (myTask!=nullptr) {
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
    }
    return myTask!=nullptr;
  }


  bool processOnePendingTaskFIFO() {
    tarch::multicore::Task* myTask = nullptr;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (not nonblockingTasks.empty()) {
        myTask = nonblockingTasks.front();
        nonblockingTasks.pop_front();
      }
    }
    if (myTask!=nullptr) {
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
    }
    return myTask!=nullptr;
  }


  bool mapOnePendingTaskOntoOMPTask() {
    tarch::multicore::Task* myTask = nullptr;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (not nonblockingTasks.empty()) {
        myTask = nonblockingTasks.front();
        nonblockingTasks.pop_front();
      }
    }
    if (myTask!=nullptr) {
      #pragma omp task priority(BackgroundConsumerPriority)
      {
        bool reschedule = myTask->run();
        if (reschedule)
          spawnTask(myTask);
        else
          delete myTask;
      }
    }
    return myTask!=nullptr;
  }


  bool mergePendingTasks(int maxTasks) {
    tarch::multicore::Task* myTask = nullptr;
    std::list< tarch::multicore::Task* > tasksOfSameType;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (not nonblockingTasks.empty()) {
        myTask = nonblockingTasks.front();
        nonblockingTasks.pop_front();
      }

      auto pp = nonblockingTasks.begin();
      while (
        myTask->canFuse()
        and
        pp!=nonblockingTasks.end()
        and
        (*pp)->getTaskType()==myTask->getTaskType()
        and
        tasksOfSameType.size()<maxTasks
      ) {
        tasksOfSameType.push_back( *pp );
        pp = nonblockingTasks.erase(pp);
      }
    }

    ::tarch::logging::Statistics::getInstance().log( MergeTasksStatisticsIdentifier, tasksOfSameType.size() );

    if (myTask!=nullptr) {
      #pragma omp task priority(BackgroundConsumerPriority)
      {
        bool stillExecuteLocally;
        if (tasksOfSameType.empty()) {
          stillExecuteLocally = true;
        }
        else {
          stillExecuteLocally = myTask->fuse(tasksOfSameType);
        }
        if (stillExecuteLocally) {
          spawnTask(myTask);
        }
      }
    }

    return myTask!=nullptr;
  }
}


/**
 * Process a few tasks from my backlog of tasks
 *
 * This routine loops through the pending tasks until either maxTasks have been
 * processed (we decrement this counter by one per loop iteration) or no tasks
 * are left anymore. In the latter case, we set maxTasks to 0 and thus terminate
 * the loop.
 *
 * Both the way we pick tasks from the queue and extract them from there and the
 * way we process the tasks depends on the current state of our task processing
 * strategy. This is encoded via the enum taskProgressionStrategy.
 */
bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier,                   tarch::multicore::getNumberOfPendingTasks() );
  if (taskProgressionStrategy==TaskProgressionStrategy::MapOntoOMPTask) {
    ::tarch::logging::Statistics::getInstance().log( MapPendingTasksOnOpenMPTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
  }

  bool  result        = false;
  while (maxTasks>0) {
    bool handledATask = false;
    switch (taskProgressionStrategy) {
      case TaskProgressionStrategy::BufferInQueue:
        handledATask = processOnePendingTaskFIFO();
        break;
      case TaskProgressionStrategy::MapOntoOMPTask:
        handledATask = mapOnePendingTaskOntoOMPTask();
        break;
      case TaskProgressionStrategy::MergeTasks:
        handledATask = mergePendingTasks(maxTasks);
        break;
    }

    if (handledATask) {
      maxTasks--;
      result = true;
    }
    else maxTasks=0;
  }

  return result;
}


/**
 * Spawns a single task in a non-blocking fashion
 *
 * @see tarch::multicore::spawnAndWait()
 * @see processPendingTasks(int)
 */
void tarch::multicore::spawnTask(Task*  task) {
  switch (taskProgressionStrategy) {
    case TaskProgressionStrategy::MapOntoOMPTask:
    case TaskProgressionStrategy::MergeTasks:
      {
        #pragma omp task priority(BackgroundConsumerPriority)
        {
          bool reschedule = task->run();
          if (reschedule)  spawnTask(task);
                      else delete task;
        }
      }
      break;
    case TaskProgressionStrategy::BufferInQueue:
      {
        #pragma omp critical (backgroundTaskQueue)
        {
          nonblockingTasks.push_back(task);
        }
      }
      break;
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}



/**
 * Process a set of tasks and wait for their completion
 *
 * Run over a set of task and wait until they are complete. Each of these tasks
 * can spawn further tasks. We do not have to wait for these guys.
 *
 * Logically, this is a plain task loop where we wait only for the direct
 * children only. Technically, we need however something
 * more sophisticated.
 *
 * - OpenMP implementations fail to "backfill" idling threads with OpenMP tasks
 *   when we hit a taskwait, and we know that the tasks within the passed
 *   container are highly ill-balanced.
 * - A lot of OpenMP tools do not support OpenMP's taskloop properly.
 * - The tasks within tasks spawn a lot of further tiny tasks. All OpenMP
 *   implementations
 *
 * The strategy is thus straightforward: Normally, we map OpenMP tasks directly
 * onto OpenMP tasks. If we hit this routine, we however start to buffer all
 * the tiny tasks that are created by the big tasks in the container in a local
 * queue. We release the tiny ones eventually after the big tasks
 * have terminated. At the same time, we map the each big task onto a proper
 * OpenMP tasks which checks towards the end of its lifetime whether its
 * colleagues have finished, too. If this is not the case, they process a few
 * of these pending tiny tasks and check again.
 *
 * <h2> Problem context </h2>
 *
 * The taskwait pragma allows the scheduler to process other tasks. This way,
 * it should keep cores busy all the time. However, several groups have 
 * reported that this is not the case. See in particular
 *
 * Jones, Christopher Duncan (Fermilab): Using OpenMP for HEP Framework Algorithm Scheduling
 * http://cds.cern.ch/record/2712271
 *
 * The presentation slides can be found at https://zenodo.org/record/3598796#.X6eVv8fgqV4. 
 *
 * This documentation clarifies that some OpenMP runtimes do (busy) waits within
 * the taskwait construct to be able to continue immediately. They do not 
 * process other tasks meanwhile. We want to support a variaty of runtimes - 
 * GCC for example is important for us due to its NVIDIA GPU support = and
 * we therefore do supply the layered implementation. Also, we need a, again
 * for our GPU/dynamic LB stuff, a variant where we have scheduler control.
 *
 * Our own ExaHyPE 2 POP review came to the same conclusion.
 *
 *
 * <h2> Implementation </h2>
 *
 * I originally thought we could just work with parallel fors. However, this
 * does not work, as we don't want to spawn our own parallel region here. We
 * expect the overarching main loop to be a parallel region. This main things
 * has to be embedded into a single/master statement, so it is only executed
 * on one thread. As a result, by the time we hit spawnAndWait, we are working
 * within a single environment. If we spawn parallel for within a single
 * environment, OpenMP will complain. It won't work, as we operate within
 * single. So I rely here a sequential loop which produces tasks.
 *
 * The implementation is straightforward: We use a counter that we set to the 
 * number of work items or number of threads; whichever is bigger. Per loop
 * iteration, we produce one task. Every task decrements this counter upon completion.
 * That is: each task knows towards the end of its lifetime how many other
 * tasks are still up and running.
 *
 * @image html layered-openmp-steps.png
 *
 * @see  processPendingTasks(int)
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  static tarch::logging::Log _log( "tarch::multicore");

  if (not tasks.empty()) {
    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );
    int       busyThreads     = NumberOfThreads;

    const char* valueFuseNum = getenv("FUSENUM");
    const char* valueFuseMax = getenv("FUSEMAX");
    int numberOfTasksThatShouldBeFused = valueFuseNum ? std::atoi(valueFuseNum) : 16;
    int maxFuseGPU =                     valueFuseMax ? std::atoi(valueFuseMax) : 1000;

    #pragma omp critical
    {
      taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
    }

    // for task loop, I need an explicit shared(busyThreads)
    for (int i=0; i<NumberOfThreads; i++) {
      #pragma omp task shared(busyThreads)
      {
        if (i<tasks.size()) {
          while (tasks[i]->run()) {
            #pragma omp taskyield
          }
          delete tasks[i];
        }

        #pragma omp atomic
        busyThreads--;

        logDebug( "spawnAndWait()", "Thread " << i << " out of " << NumberOfThreads << " threads is done (still " << busyThreads << " threads busy for " << tasks.size() << " task items)" );

        while (
          busyThreads>0
          and
          not nonblockingTasks.empty()
          and
          // Without this yield, the code will deadlock if you have more trees
          // than cores. As the first p trees on p cores will finish and then
          // poll. The other >p trees/tasks will starve
          busyThreads<tarch::multicore::Core::getInstance().getNumberOfThreads()
        ) {
          if (nonblockingTasks.size()>=numberOfTasksThatShouldBeFused and maxFuseGPU>0) {
            #if PeanoDebug>=2
            logInfo( "spawnAndWait()", "merge " << numberOfTasksThatShouldBeFused << " tasks" );
            #endif
            mergePendingTasks(numberOfTasksThatShouldBeFused);

            #pragma omp atomic
            numberOfTasksThatShouldBeFused *= 2;
            
            #pragma omp atomic
            maxFuseGPU--; //
          }
          else if (nonblockingTasks.size()>numberOfTasksThatShouldBeFused) {
            tarch::multicore::processPendingTasks( numberOfTasksThatShouldBeFused );
          }
          else {
            #pragma omp taskyield
          }
        }
      }

      ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
    }
    #pragma omp taskwait

    while (nonblockingTasks.size()>=numberOfTasksThatShouldBeFused and maxFuseGPU>0) {
      #if PeanoDebug>=2
      logInfo( "spawnAndWait()", "merge " << numberOfTasksThatShouldBeFused << " tasks" );
      #endif
      mergePendingTasks(numberOfTasksThatShouldBeFused);
      #pragma omp atomic
      maxFuseGPU--; //
    }

    // Release all the remaining tasks as proper OpenMP tasks
    #pragma omp critical
    {
      taskProgressionStrategy = TaskProgressionStrategy::MapOntoOMPTask;
    }

    // This is to avoid that we run into OpenMP deadlocks
    if ( tarch::multicore::Core::getInstance().getNumberOfThreads()>1 ) {
      #if PeanoDebug>=2
      logInfo( "spawnAndWait()", "release " << nonblockingTasks.size() << " tasks as proper OpenMP tasks" );
      #endif
      tarch::multicore::processPendingTasks(nonblockingTasks.size());
    }
  }
}


void tarch::multicore::yield() {
  #pragma omp taskyield
}


/**
 * Process one particular task 
 * 
 * If we search for a particular task, we typically should search
 * LIFO. Therefore, I change the general processing pattern and
 * continue.
 */
bool tarch::multicore::processTask(int number) {
  bool foundTask = tarch::multicore::processPendingTasks(1);
  if (not foundTask) {
    #pragma omp taskyield
  }
  return foundTask;
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#endif
