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
#include "../Tasks.h"

#include "tarch/logging/Statistics.h"


namespace {
  std::list<tarch::multicore::Task* > nonblockingTasks;
  std::mutex                           taskQueueMutex;
  const int StandardPriority           = 16;
  const int BackgroundConsumerPriority = 1;

  /**
   * Determines how to handle the tasks dumped into nonblockingTasks.
   */
  enum class TaskProgressionStrategy {
    BufferInLIFOQueue,
    BufferInFIFOQueue,
    MapOntoOMPTask,
    /**
     * This is usually always the default. Once we cannot deploy stuff
     * anymore (or should not deploy), the marker switches to BufferInQueue.
     */
    DeployToRemoteDevice,
    MergeTasks
  };

  /**
   * This flag/strategy is toggled by tarch::multicore::spawnAndWait(). The
   * flag determines how
   * tarch::multicore::processPendingTasks(int) processes the tasks.
   */
  TaskProgressionStrategy  taskProgressionStrategy = TaskProgressionStrategy::BufferInFIFOQueue;

  const std::string MapPendingTasksOnOpenMPTasksStatisticsIdentifier( "tarch::multicore::map-pending-tasks-onto-openmp-tasks");
  const std::string MergeTasksStatisticsIdentifier( "tarch::multicore::merge-tasks");
}


/**
 * Process a few tasks from my backlog of tasks and eventually trigger further consumes.
 *
 * After we have run through the given number of tasks, we examine whether we have
 * processed something (or have been called with argument 0) or the number of tasks
 * lingering in the background has even increased. In these cases, we span new OpenMP
 * tasks with low priority that continue to run through these pending jobs.
 */
bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier,                   tarch::multicore::getNumberOfPendingTasks() );
  if (taskProgressionStrategy==TaskProgressionStrategy::MapOntoOMPTask) {
    ::tarch::logging::Statistics::getInstance().log( MapPendingTasksOnOpenMPTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
  }
  /*
            const std::string MapPendingTasksOnOpenMPTasksStatisticsIdentifier( "tarch::multicore::map-pending-tasks-onto-openmp-tasks");
            const std::string MergeTasksStatisticsIdentifier( "tarch::multicore::merge-tasks");
  */

  bool  result        = false;

  while (maxTasks>0) {
    Task* myTask = nullptr;

    switch (taskProgressionStrategy) {
      case TaskProgressionStrategy::DeployToRemoteDevice:
        {
          #pragma omp critical (backgroundTaskQueue)
          {
            taskProgressionStrategy = TaskProgressionStrategy::DeployToRemoteDevice;
          }
        }
        break;
      case TaskProgressionStrategy::MapOntoOMPTask:
      case TaskProgressionStrategy::BufferInLIFOQueue:
        {
          #pragma omp critical (backgroundTaskQueue)
          {
            if (not nonblockingTasks.empty()) {
              myTask = nonblockingTasks.back();
              nonblockingTasks.pop_back();
            }
          }
        }
        break;
      case TaskProgressionStrategy::BufferInFIFOQueue:
        {
          #pragma omp critical (backgroundTaskQueue)
          {
            if (not nonblockingTasks.empty()) {
              myTask = nonblockingTasks.front();
              nonblockingTasks.pop_front();
            }
          }
        }
        break;
      case TaskProgressionStrategy::MergeTasks:
        {
          #pragma omp critical (backgroundTaskQueue)
          {
            if (not nonblockingTasks.empty()) {
              myTask = nonblockingTasks.front();
              nonblockingTasks.pop_front();
            }
            taskProgressionStrategy = TaskProgressionStrategy::BufferInFIFOQueue;
          }
          break;

        }
        break;
    }

    if (myTask!=nullptr) {
      switch (taskProgressionStrategy) {
        case TaskProgressionStrategy::BufferInLIFOQueue:
        case TaskProgressionStrategy::BufferInFIFOQueue:
        case TaskProgressionStrategy::MergeTasks:
          {
            bool requeue = myTask->run();
            if (requeue)
              spawnTask( myTask );
            else
              delete myTask;
          }
          break;
        case TaskProgressionStrategy::MapOntoOMPTask:
        case TaskProgressionStrategy::DeployToRemoteDevice:
          {
            #pragma omp task priority(BackgroundConsumerPriority)
            {
              bool reschedule = myTask->run();
              if (reschedule)
                spawnTask(myTask);
              else
                delete myTask;
            }
          }
          break;
      }
      maxTasks--;
      result = true;
    }
    else {
      maxTasks = 0;
    }
  }

  return result;
}


/**
 * Spawns a single task in a non-blocking fashion
 *
 * Non-blocking means that the routine should return immediately. One might
 * think that this is a great match for OpenMP tasks. However, spawnTasks()
 * is (typically) called by a taskgroup. New OpenMP tasks thus would become descendants
 * and eventually have to complete before the taskgroup completes. This is
 * not what we want: If we spawn a task this way, it really should go into
 * the background and be processed any time later when cores become idle.
 *
 * It is also no option to make the spawning part a task loop where we
 * wait only for the direct children: On the production side, everything
 * then is fine, i.e. we parallel loop fragments mapped onto tasks spawn
 * further task but does not wait for them logically. There are however
 * two problems:
 *
 * - Many scheduler do depth-first and may suspend task execution whenever
 *   a new task is created that is a subtask in a situation where no threads
 *   are idle.
 * - We have other parallel loops which eventually run for the outcome of
 *   the background tasks. As OpenMP has no mechanism alike "call a task
 *   with a lower priority that you haven't done yet", we yield. Yieling
 *   however might mean - if N of these parallel tasks that wait for
 *   background tasks run - that background tasks trigger each other, but
 *   the actual compute task never is invoked.
 *
 * So I simply enqueue these tasks in a local helper data structure and
 * return for the time being. All spawned (background) tasks are put into
 * this local queue. The actual processing then is done via
 * processPendingTasks().
 */
void tarch::multicore::spawnTask(Task*  job) {
   #pragma omp critical (backgroundTaskQueue)
   {
     nonblockingTasks.push_back(job);
   }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}



/**
 * Process a set of tasks and wait for their completion
 *
 * I originally thought we could just work with parallel fors. However, this
 * does not work, as we don't want to spawn our own parallel region here. We
 * expect the overarching main loop to be a parallel region. This main things
 * has to be embedded into a single/master statement, so it is only executed
 * on one thread. As a result, by the time we hit spawnAndWait, we are working
 * within a single environment. If we spawn parallel for within a single
 * environment, OpenMP will complain. It won't work, as we operate within
 * single. So I rely here on explicit tasking. Other threads will steal away
 * the guys we create.
 *
 * The taskloop requires an explicit nogroup statement, as we don't want the
 * loop to wait for all descendents. We only want to wait for direct 
 * children. Therefore, we take away the group property and add an explicit
 * taskwait.
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
 * <h2> Implementation </h2>
 *
 * The implementation is straightforward: We use a counter that we set to the 
 * number of work items. Every task decrements this counter upon completion.
 * That is: each task knows towards the end of its lifetime how many other
 * tasks are still up and running. While there are some other tasks, it can
 * grab a task and process it.
 *
 * Grabbing only one task at a time is not efficient. The overhead of the 
 * lookups is too big. Therefore, we use a variable max_tasks logcally. It
 * is initialised by the very first task that completes: It analyses the 
 * number of available background tasks at the time, and initialises 
 * max_tasks with this number divided by the number of high-level tasks that
 * have to be completed. This way, we add a dynamic touch to the whole thing. 
 *
 * <h2> Bugs </h2>
 *
 * Due to the above scheduling policy, we have frequently observed that our
 * code deadlocks with GCC if we use more OMP_NUM_THREADS than physical 
 * threads. Notably, if hyperthreading is used, too. In this case, our 
 * domain traversal tasks from tasks seem to starve.
 *
 *
 *
 * @see  processPendingTasks(int).
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  if (not tasks.empty()) {
    static int NumberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();

    int busyThreads = NumberOfThreads;

    #pragma omp critical
    {
      taskProgressionStrategy = TaskProgressionStrategy::DeployToRemoteDevice;
    }

    // I always occupy all threads, though only the first tasks.size()
    // will actually get work. The others will immediately crawl through
    // the pending tasks.
    #pragma omp taskloop nogroup priority(StandardPriority) shared(busyThreads)
    for (int i=0; i<NumberOfThreads; i++) {
      if ( i<tasks.size() ) {
        while (tasks[i]->run()) {
          #pragma omp yield
        }
        delete tasks[i];
      }

      #pragma omp atomic
      busyThreads--;

      while (busyThreads>0 and not nonblockingTasks.empty()) {
        tarch::multicore::processPendingTasks(nonblockingTasks.size()/2);
      }

      ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
    }
    #pragma omp taskwait
  }

  #pragma omp critical
  {
    taskProgressionStrategy = TaskProgressionStrategy::BufferInFIFOQueue;
  }

  tarch::multicore::processPendingTasks(nonblockingTasks.size());

  #pragma omp critical
  {
    taskProgressionStrategy = TaskProgressionStrategy::DeployToRemoteDevice;
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
  #pragma omp critical
  {
    taskProgressionStrategy = TaskProgressionStrategy::BufferInLIFOQueue;
  }

  return tarch::multicore::processPendingTasks(1);
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#endif
