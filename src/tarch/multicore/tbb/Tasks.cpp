#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"



#if defined(SharedTBB)
#include <tbb/task_group.h>
#include <tbb/task_arena.h>

namespace {
  tarch::multicore::BooleanSemaphore  suspensionPointSemaphore;

  const std::string BSPConcurrencyLevelStatisticsIdentifier( "tarch::multicore::bsp-concurrency-level" );

  /**
   * Seems that there is no atomic in tbb. So I use the standard one.
   */
  std::atomic<int> nonblockingTasksCounter          = 0;

  /**
   * We seem to need this task group. Has to be investigated.
   * 
   * @see spawnTask()
   * @see spawnAndWaitAsTaskLoop()
   */
  ::tbb::task_group backgroundTaskGroup;

  /**
   * Native mapping of a task loop onto a SYCL/oneTBB loop
   *
   *
   * # Design
   *
   * In principle, the TBB/SYCL world offers us two ways on the CPU/host to pipe tasks
   * into the tasking system: We can either rely on bespoke tasking interfaces, or we
   * can use a queue which we pin to the host. The latter is the mechanism that we have
   * to use on GPUs: We throw tasks into a GPU queue and wait until these tasks have
   * completed. I would have liked to use the queue concept on the host, too. From a
   * source code's perspective, it would be great to stick to one paradigm everywhere.
   *
   * Using queues on the host however is off the table, as tasks within queues may not
   * have side effects, i.e. access and modify global variables, or use virtual function
   * calls. Our code base however relies heavily on virtual function calls, unless
   * kernels are explicitly tailored towards particular solvers. So no queues on the CPU.
   *
   * With SYCL/TBB, this leaves us with two options on the host side: We could either
   * use the C++ API, or we could use something TBB-specific. In principle, the C++
   * front-end should, behind the scenes, feed into TBB. Under the hood, we therefore
   * work with TBB anyway. TBB however offers some nice constructs such as the taskGroup
   * which are convenient. Furthermore, the TBB-specific constructs offer two more
   * advantages:
   * - They ensure that the BSP section is really mapped onto tasks and not onto
   *   heavy-weight threads.
   * - They offer the opportunity to make the interna NUMA-aware as they explicitly
   *   integrate with TBB's arena concept.
   *
   * The high-level architecture thus reads as follows: Peano uses a BSP-like structure
   * on the highest level. This BSP structure is mapped onto TBB's taskGroup: Every
   * BSP thread (this is typically a subdomain traversal) is mapped onto one task within
   * the group, we trigger all of these tasks (aka traversals) and then wait for the
   * task group to terminate.
   *
   * Each task within the task group (aka BSP thread - though these logical threads are
   * internally mapped onto lightweight tasks) can spawn additional tasks. As part of
   * the enclave concept, we don't have to wait for these children tasks at the end
   * of the BSP section. Therefore, the BSP tasks enqueue their new children into a
   * separate task group (see spawnTask()) and ``forget'' them.
   *
   * This design with two task groups is very convenient. The synchronisation within the
   * BSP task group is straightforward: We fire our tasks (which might spawn additional
   * tasks which we ignore), and we wait for all BSP tasks to terminate. Whenever we
   * wait for some task from the second task group, we simply empty the second task
   * group.
   *
   * # Runtime assumptions
   *
   * We expect that TBB can handle two task groups. One of them is persistent (the one
   * holding these enclave tasks), while the other type of task group is created
   * on-the-fly whenever we hit a BSP code segment. We also expect TBB to use both
   * task groups to keep all threads idle and to give the temporary BSP-type task
   * group higher priority: the BSP threads should always come first. Only once they
   * have terminated and freed their cores, TBB should step in and ``backfill'' the
   * idling cores with tasks from the other background task group.
   *
   * @see spawnTask()
   *
   */
  void spawnAndWaitAsTaskLoop(
    const std::vector< tarch::multicore::Task* >&  tasks
  ) {
    assertion( not tasks.empty() );

    // is implicitly executed within root arena, as the invoking task is
    // run within root arena
    ::tbb::task_group taskGroup;
    for (auto& p: tasks) {
      taskGroup.run(
        [&tasks,p]() -> void {
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, 1.0, true );
          while (p->run()) {}
          delete p;
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -1.0, true );
        }
      );
    }
    taskGroup.wait();
  }


  /**
   * Aggressive BSP-type variant to run through tasks which ensures high
   * throughput
   *
   * This is a straightforward translation of the OpenMP high-throughput
   * realisation.
   *
   * @see spawnAndWaitAsTaskLoop() for realisation details re TBB/SYCL.
   */
  void spawnAndWaitAsExplicitTasksWithPolling(
    const std::vector< tarch::multicore::Task* >&  tasks,
    tarch::multicore::orchestration::Strategy*     strategy
  ) {
    assertion( not tasks.empty() );

    const int NumberOfThreads = std::max( tarch::multicore::Core::getInstance().getNumberOfThreads(), static_cast<int>(tasks.size()) );

    std::atomic<int>       pendingTasks  = NumberOfThreads;
    std::atomic<int>       spawnedTasks  = 0;

    ::tbb::task_group taskGroup;
    for (int i=0; i<NumberOfThreads; i++) {
      // is implicitly executed within root arena
      taskGroup.run(
        [&spawnedTasks,&pendingTasks,&tasks,i,NumberOfThreads,&strategy] () -> void {
          spawnedTasks++;

          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, 1.0, true );
          if (i<tasks.size()) {
            while (tasks[i]->run()) {}
            delete tasks[i];
          }
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -1.0, true );
  
          pendingTasks--;

          progressTasksWithinSpawnAndWaitAsExplicitTasksWithPolling(
            pendingTasks,
            spawnedTasks
          );
        }
      );
    }
    taskGroup.wait();
  }
}


/**
 * Read spawnAndWaitAsTaskLoop() first.
 *
 * @see spawnAndWaitAsTaskLoop()
 */
void tarch::multicore::native::spawnTask(Task*  job) {
  nonblockingTasksCounter++;

  // Does not work, as we have virtual functions, too.
/*
  backgroundTaskQueue.submit([&] (sycl::handler& commandGroupHandler) {
    commandGroupHandler.single_task(
      [job] () -> void {
        //nonblockingTasksCounter--;
      }
    );
  });
*/


  // Serial version
/*
  while (job->run()) {}
  delete job;
  nonblockingTasksCounter--;
*/

  backgroundTaskGroup.run([&,job]{
    while (job->run()) {}
    delete job;
    nonblockingTasksCounter--;
  });
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
  backgroundTaskGroup.run([&,myTask,copyOfTasksOfSameType]{
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType,device);
     if (stillExecuteLocally) {
       tarch::multicore::native::spawnTask(myTask);
     }
     else {
       delete myTask;
     }
  });
}


void tarch::multicore::native::deferRunningThreadAndContinueWithOtherTasks() {
  backgroundTaskGroup.wait();
}


#endif
