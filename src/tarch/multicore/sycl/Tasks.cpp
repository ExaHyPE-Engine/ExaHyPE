#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"

#include "tarch/mpi/Rank.h"

#include "tarch/logging/Statistics.h"


#if defined(SharedSYCL)

#include <atomic>
#include <execution>
#include <future>
#include <list>

#include <oneapi/tbb/task_arena.h>

namespace {
  tarch::multicore::BooleanSemaphore  suspensionPointSemaphore;

  std::atomic<int> nonblockingTasksCounter          = 0;

  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");

  /**
   * Additional subarena
   */
  oneapi::tbb::task_arena backgroundTaskArena( oneapi::tbb::task_arena::automatic, 1, oneapi::tbb::task_arena::priority::low  );
  oneapi::tbb::task_arena fusedTaskArena(      oneapi::tbb::task_arena::automatic, 1, oneapi::tbb::task_arena::priority::high );

  /**
   * In oneTBB, we can suspend a task and will get back a tag to wake up the
   * thread later on.
   */
  std::list< oneapi::tbb::task::suspend_point >    suspendedTags;

  /**
   * If you yield(), the SYCL implementation does not yield. It suspends the
   * active thread. This implies that we have to wake them up again.
   */
  void wakeUpSuspendedTasks() {
    tarch::multicore::Lock lock( suspensionPointSemaphore );

    for (auto& p: suspendedTags ) {
      oneapi::tbb::task::resume(p);
    }

    suspendedTags.clear();
  }

  /**
   * Native mapping of a task loop onto a task loop
   *
   * I found that quite a lot of OpenMP implementations realise the taskwait at the 
   * end as busy wait. This is totally annoying as it might lead to deadlocks whenever
   * a task has a taskyield internally and/or there are more tasks than threads.
   */
  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    // important, otherwise taskwait synchronises
    assertion( not tasks.empty() );

    // is implicitly executed within root arena, as the invoking task is
    // run within root arena
    oneapi::tbb::task_group taskGroup;
    for (auto& p: tasks) {
      taskGroup.run(
        [&tasks,p]() -> void {
          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
          while (p->run()) {}
          delete p;
          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
        }
      );
    }
    taskGroup.wait();
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * This implementation variant follows the patterns described in the 2021
   * IWOMP paper by H. Schulz et al, but it is way more advanced. An important
   * paper to read in the context of the present solution is also
   * https://www.osti.gov/pages/servlets/purl/1465188. Note in particular the
   * example in Figure 3.
   *
   * # Realisation
   *
   * The idea is that we spawn one task per bsp segment (stored within tasks).
   * If a task terminates, we do not terminate immediately but first check if
   * all the other tasks have already finished, too. If they haven't, we
   * process a few pending tasks from the queue and then check again. We
   * prioritise throughput over an immediate termination of the BSP section.
   *
   * As we might have fewer tasks or more tasks than cores, we take the maximum
   * of both of them. If the a task is spawned that does not correspond to an
   * entry in tasks, i.e. if tasks has fewer entries than threads, this task
   * will not process a BSP task but immediately start to run through the
   * pending tasks.
   *
   * The devil is in the details when it comes to the actual while loop that
   * processes pending tasks:
   *
   * - The while loop should terminate once all tasks of the BSP section have
   *   done their core work. We introduce a shared atomic variable pendingTasks
   *   which counts down. Whenever a BSP task has finished its core work, it
   *   decrements it. The while loop continues if pendingTasks reduces to zero.
   * - If we have more tasks than threads, we should not allow a BSP task to
   *   process pending tasks. It might become so busy with processing these guys
   *   that it actually starves other BSP-type tasks.
   * - We really do not know in OpenMP when a ready task is processed. The task
   *   loop might trigger all tasks, do them, and then wait. This is the kind
   *   pattern that we want to have. However, it could also be that two tasks
   *   start, the machine then does busy waiting in the waitall and the other
   *   tasks do not start. Therefore, it is important that we introduce a second
   *   atomic counter which counts how many tasks have actually kicked off. If
   *   and only if all tasks that are ready had a chance to run, we know that
   *   all BSP stuff is now in work. In this case, we can also enable the
   *   polling. But not before.
   *
   * # System-specific details
   *
   * NVIDIA's nvcc does not support the taskloop. It would be easy to replace
   * the taskloop with a for loop and tasks within the loop body. But I decided
   * to strive for a clean code.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    std::atomic<int>       pendingTasks  = NumberOfThreads;
    std::atomic<int>       spawnedTasks  = 0;

    oneapi::tbb::task_group taskGroup;
    for (int i=0; i<NumberOfThreads; i++) {
      // is implicitly executed within root arena
      taskGroup.run(
        [&spawnedTasks,&pendingTasks,&tasks,i,NumberOfThreads,&strategy] () -> void {
          spawnedTasks++;

          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, 1.0, true );
          if (i<tasks.size()) {
            while (tasks[i]->run()) {
              wakeUpSuspendedTasks();
            }
            delete tasks[i];
          }

          ::tarch::logging::Statistics::getInstance().inc( BSPTasksStatisticsIdentifier, -1.0, true );
  
          pendingTasks--;

          while (
            pendingTasks>0
            and
            // Without this yield, the code will deadlock if you have more trees
            // than cores. As the first p trees on p cores will finish and then
            // poll. The other >p trees/tasks will starve
            pendingTasks<tarch::multicore::Core::getInstance().getNumberOfThreads()
            and
            spawnedTasks==NumberOfThreads
          ) {
            int numberOfPendingTasks = tarch::multicore::getNumberOfPendingTasks();

            if ( numberOfPendingTasks>0 ) {
              std::pair<int,int> fusion = strategy->getNumberOfTasksToFuseAndTargetDevice();

              if (fusion.first>0 and numberOfPendingTasks>=fusion.first) {
                tarch::multicore::internal::fusePendingTasks(fusion);
              }
              else {
                int numberOfTasksToProcess = std::max(1,numberOfPendingTasks/2);
                tarch::multicore::processPendingTasks( numberOfTasksToProcess );
              }
            }
            else {
              wakeUpSuspendedTasks();
              #if defined(Parallel)
              // Allow MPI to make progress. We otherwise might starve MPI
              int flag;
              MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, MPI_STATUS_IGNORE );
              #endif
            }
          }
        }
      );
    }
    taskGroup.wait();
  }
}


void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasksCounter++;

  backgroundTaskArena.enqueue(
    [job] () -> void {
      while (job->run()) {
        wakeUpSuspendedTasks();
      }
      delete job;
      nonblockingTasksCounter--;
      wakeUpSuspendedTasks();
    }
  );
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
  const std::vector< Task* >&  tasks,
  orchestration::Strategy*     strategy
) {
  if (strategy->processPendingTasksWhileWaitingInBSPSection()) {
    spawnAndWaitAsExplicitTasksWithPolling(tasks,strategy);
  }
  else {
    spawnAndWaitAsTaskLoop(tasks);
  }
}


/**
 * In TBB, you should not use a yield. Instead, you can can suspend the running
 * task and you get back a suspension_point tag. The latter is to be used to wake
 * the thread again.
 */
void tarch::multicore::native::yield() {
/*
  static tarch::logging::Log _log( "tarch::multicore::native" );

  tarch::multicore::Lock lock( suspensionPointSemaphore );
  int remainingConcurrency = nonblockingTasksCounter-suspendedTags.size()-1;
  bool suspendThisTask = remainingConcurrency > tarch::multicore::Core::getInstance().getNumberOfThreads()
                     and suspendedTags.size() < tarch::multicore::Core::getInstance().getNumberOfThreads()-1; // keep one alive
  lock.free();

  if (suspendThisTask) {
    logInfo( "yield()", "yield() suspends thread (nonblocking-tasks=" << nonblockingTasksCounter << ",suspended-tasks=" << suspendedTags.size() << ",threads=" << tarch::multicore::Core::getInstance().getNumberOfThreads() << "), remaining-concurrency=" << remainingConcurrency );
    oneapi::tbb::task::suspend([&] (oneapi::tbb::task::suspend_point tag) {
      suspendedTags.push_back( std::move(tag) );
      lock.free();
    });
  }

  wakeUpSuspendedTasks();
*/
}


int tarch::multicore::native::getNumberOfPendingTasks() {
  return nonblockingTasksCounter;
}


/**
 * It is not clear what happens if I use tasksOfSameType directly within the task.
 * It is clear that a pure reference cannot be used within a task, as the task uses
 * firstprivate by default. Therefore, it would copy something which ceases to exist.
 * It is not clear what happens with const references, but we had some memory issues
 * with the newer Intel/LLVM versions. Therefore, I manually copy the list and pass
 * this one into fuse(). The firstprivate annotation is not required, as this is the
 * default. But I leave it in here to highlight that three variables are copied,
 * but tasksOfSameType is not copied and not used within the task.
 */
void tarch::multicore::native::processFusedTask(Task*  myTask, const std::list< tarch::multicore::Task* >& tasksOfSameType, int device) {
  std::list< tarch::multicore::Task* > copyOfTasksOfSameType = tasksOfSameType;
  fusedTaskArena.enqueue(
    [myTask,copyOfTasksOfSameType,device]() -> void {
      bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType,device);
      if (stillExecuteLocally) {
        tarch::multicore::native::spawnTask(myTask);
      }
      else {
        delete myTask;
      }
      wakeUpSuspendedTasks();
    }
  );
}

#endif
