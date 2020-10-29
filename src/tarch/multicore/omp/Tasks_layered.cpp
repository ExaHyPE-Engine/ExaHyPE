#include "tarch/Assertions.h"
#include "../Tasks.h"
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

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier,        tarch::multicore::getNumberOfPendingTasks() );

  bool  result        = false;

  // TODO change such that we process a whole bunch of tasks at a time
  while (maxTasks>0) {
    Task* myTask = nullptr;
    std::vector<Task*> work;
    #pragma omp critical (backgroundTaskQueue)
    {
      if (nonblockingTasks.empty()) {
        maxTasks = 0;
      }
      else {
         while (not nonblockingTasks.empty() and (work.size() < std::min(maxTasks, int(nonblockingTasks.size())) ) )
         {
            myTask = nonblockingTasks.front();
            nonblockingTasks.pop_front();
            work.push_back(myTask);
         }
      }
    }
    //std::cerr << std::this_thread::get_id() << " will process " << work.size() << " tasks while maxTasks is " << maxTasks <<"\n";

   if (myTask!=nullptr)
    {
       while (work.size()>0)
       {
          myTask = work.back();
          bool requeue = myTask->run();
          if (requeue) {
            spawnTask( myTask );
          }
          else {
            delete myTask;
          }
          work.pop_back();
          maxTasks--;
          result = true;

       }
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
 * In principle, the processing of the tasks is a simple task group. However,
 * we may assume that that tasks are pretty imbalanced, so it would be nice
 * if the processing of some background tasks could slot in. Therefore, I
 * first spawn to tasks: The first one issues one task per item in the vector
 * tasks. The other task is kind of degenerated: It issues core consumer tasks.
 * On the highest level, we wait for both tasks to terminate. The one that
 * spawns further consumer tasks will actually return almost immediately. The
 * traversal task on the other hand will wait for quite a while as it itself
 * hosts a taskwait waiting for the mesh traversal tasks to get there.
 *
 * @see  processPendingTasks(int).
 */
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
     int max_tasks = 50;
     // NOTE: this is likely not performant
     if (std::getenv("MAXTASKS") != nullptr) max_tasks = atoi(std::getenv("MAXTASKS"));

      int nnn(tasks.size());
      #pragma omp taskloop nogroup priority(StandardPriority) shared(nnn)
      for (int i=0; i<tasks.size(); i++) {
        while (tasks[i]->run()) {}
        delete tasks[i];
        #pragma omp atomic 
        nnn--;

        if (omp_get_thread_num() == 0)
        {
           std::cerr << "# BG tasks: " << nonblockingTasks.size() << "\n";
        }
        
        while (nnn>0 && not nonblockingTasks.empty())
        {
           tarch::multicore::processPendingTasks(max_tasks);
        }
      }
     #pragma omp taskwait
   // #pragma omp task
   // {
   //   spawnTaskConsumer(
   //     tarch::multicore::Core::getInstance().getNumberOfThreads()
   //   );
   // }
    // std::cerr << omp_get_thread_num() << " done waiting " <<  nnn << " \n";
 // }
}


void tarch::multicore::yield() {
  #pragma omp taskyield
}

//bool tarch::multicore::processTask(int number) {
//     yield();
//    return false;
//}

bool tarch::multicore::processTask(int number) {

  // Iterate backwards through list
  Task* myTask = nullptr;
  taskQueueMutex.lock();
  for (auto it = nonblockingTasks.end(); it !=nonblockingTasks.begin();)
  {
    --it;
    if ((*it)->getTaskId() == number) {
       myTask = (*it);
       nonblockingTasks.erase(it);
       it=nonblockingTasks.begin();
    }
  }

  taskQueueMutex.unlock();

  if (myTask != nullptr)
  {
    while (myTask->run()) {};
    delete myTask;
    return true;
  }
  else
  {
    return false;
  }
}

int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#endif
