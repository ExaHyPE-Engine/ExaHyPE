#include "tarch/multicore/Tasks.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"

#if defined(SharedTBB)
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <oneapi/tbb/task_arena.h>

namespace {
  tarch::multicore::BooleanSemaphore suspensionPointSemaphore;

  ::oneapi::tbb::task_arena defaultTaskArena;

  /**
   * Task arena for background tasks
   *
   * TBB models the world in task arenas, with the global one having the
   * default priority. If you don't use task arenas yourself, then you
   * will always submit to the default arena, i.e. each and every task
   * will have the same priority. We want to avoid such default behaviour
   * and instead use the three priorities offered by TBB.
   */
  ::oneapi::tbb::task_arena backgroundTaskArena(
    ::oneapi::tbb::task_arena::automatic,    // max_concurrency
    1,                                       // reserved_for_masters
    ::oneapi::tbb::task_arena::priority::low // priority a_priority
  );

  ::oneapi::tbb::task_arena highPriorityTaskArena(
    ::oneapi::tbb::task_arena::automatic,     // max_concurrency
    1,                                        // reserved_for_masters
    ::oneapi::tbb::task_arena::priority::high // priority a_priority
  );

  /**
   * Fiddle out which arena to use for a job
   */
  ::oneapi::tbb::task_arena* getArena(::tarch::multicore::Task* task) {
    ::oneapi::tbb::task_arena* result = &defaultTaskArena;

    if (task->getPriority() > tarch::multicore::Task::DefaultPriority) {
      result = &highPriorityTaskArena;
    } else if (task->getPriority() < tarch::multicore::Task::DefaultPriority) {
      result = &backgroundTaskArena;
    }

    return result;
  }
} // namespace


/**
 * Native mapping of a task loop onto a SYCL/oneTBB loop
 *
 * In TBB, the convenient way to model fork-join parallelism is the creation of
 * a task group. We can then assign all tasks to that task group and eventually
 * wait for all of its tasks to terminate.
 *
 * Each task within the task group (aka BSP thread - though these logical threads are
 * internally mapped onto lightweight tasks) can spawn additional tasks. As part of
 * the enclave concept, we don't have to wait for these children tasks at the end
 * of the BSP section. Therefore, the BSP tasks enqueue their new children into a
 * separate task group/arena (see spawnTask()) and ``forget'' them.
 *
 *
 * ## Task arenas and task priorities
 *
 * I do not explicitly submit the new task group into a task arena, i.e. I use
 * the task arena of the surrounding task. Peano uses nested parallelism. We
 * therefore might submit spawnAndWaitAsTaskLoop() for different areas with
 * different priorities.
 *
 * This realisation remark clarifies that we do not consider task priorities here.
 *
 * If we worked with a task arena explicitly, we would have to write
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   ::tbb::task_group taskGroup;
 *   for (auto& p : tasks) {
 *     defaultTaskArena.execute( [&]{
 *       taskGroup.run([&tasks, p]() -> void {
 *         while (p->run()) {
 *           tarch::multicore::Core::getInstance().yield();
 *         }
 *         delete p;
 *       });
 *     });
 *   }
 *
 *   defaultTaskArena.execute( [&]{
 *     taskGroup.wait();
 *   });
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *
 * @see spawnTask()
 *
 */
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  ::tbb::task_group taskGroup;
  for (auto& p : tasks) {
    taskGroup.run([&tasks, p]() -> void {
      while (p->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete p;
    });
  }

  taskGroup.wait();
}


void tarch::multicore::native::spawnTask(Task* job, const TaskNumber& in, const TaskNumber& out) {
  tarch::multicore::native::spawnTask(job);
}


/**
 * Spawn a single task
 *
 * Spawn a single task. This spawn is not a classic tree-based/nested
 * parallelism but instead is used within Peano to realise a producer-consumer
 * pattern. As a result, we cannot simply spawn the task as a child, but we
 * have to enqueue it as a free task.
 *
 * There are two options to do so. On the one hand, we could use a global task
 * group, which we use to always "supervise" all ready tasks that are produced.
 * On the other hand, we could use TBB's arena mechanism and spawn the produced
 * tasks directly as new roots into an arena of our choice.
 *
 * The advantage of the task group approach is that we can always explicitly
 * empty all (background) tasks by invoking a wait() on the task group.
 * However, relying (solely) on an explicit task group strips us off the
 * opportunity to work with different priorities, and it is not really in line
 * with the TBB vision, where a task group is a helper construct to logically
 * order tasks which have to synchronise at the end. Therefore, I decided to
 * abandon the task group concept and instead work with task arenas.
 *
 *
 * ## Priorities
 *
 * TBB does not support proper task priorities anymore.
 * Instead, its arenas have priorities. This does not match Peano, where each
 * and every task could have a different priority. Therefore, we create three
 * explicit task arenas (one per TBB priority) and check the priorities handed
 * in for the task: If the priority is higher than the default priority, we
 * enqueue a task in the high priority arena. For low priority we do the same
 * kind of check. All the default queues go into TBB's default arena.
 *
 * @see spawnAndWaitAsTaskLoop()
 */
void tarch::multicore::native::spawnTask(Task* job) {
  getArena(job)->enqueue([&, job] {
    while (job->run()) {
    }
    delete job;
  });
}

/**
 * Process a fused task (an assembly of multiple tasks of the same type)
 *
 * It is not clear what happens if I use tasksOfSameType directly within the task.
 * It is clear that a pure reference cannot be used within a task, as the task uses
 * firstprivate by default. Therefore, it would copy something which ceases to exist.
 * It is not clear what happens with const references, but we had some memory issues
 * with the newer Intel/LLVM versions. Therefore, I manually copy the list and pass
 * this one into fuse(). The firstprivate annotation is not required, as this is the
 * default. But I leave it in here to highlight that three variables are copied,
 * but tasksOfSameType is not copied and not used within the task.
 */
void tarch::multicore::native::processFusedTask(
  Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device
) {
  std::list<tarch::multicore::Task*> copyOfTasksOfSameType = tasksOfSameType;
  getArena(myTask)->execute([&, myTask, copyOfTasksOfSameType] {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask);
    } else {
      delete myTask;
    }
  });
}


void tarch::multicore::native::spawnTaskWithDependencies(Task* job, int inDependency, int taskNumber) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::spawnTaskWithDependencies(
  Task* job, const std::vector<int>& inDependencies, int taskNumber
) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::waitForTasks(const std::vector<int>& inDependencies) {}

void tarch::multicore::native::waitForTask(int taskNumber) {}
#endif
