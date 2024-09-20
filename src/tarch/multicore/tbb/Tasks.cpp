// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Tasks.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/services/ServiceRepository.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"

#if defined(SharedTBB)
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/concurrent_hash_map.h>

#include "tbb/dynamic_task_graph.h"
#include "tbb/dynamic_task_graph_node.h"
#include "tbb/dynamic_task_graph_node_repository.h"


namespace {
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
   *
   * This operation analyses the task's priority, compares it to the default
   * priority and then returns a pointer to the fitting task arena.
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

  ::oneapi::tbb::dynamic_task_graph_node_repository  taskGraphNodeRepository;
  ::oneapi::tbb::dynamic_task_graph                  taskGraph(defaultTaskArena);
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


int tarch::multicore::native::getNumberOfPendingTasks() {
  return taskGraph.size();
}


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  if (taskNumber==NoOutDependencies) {
    ::oneapi::tbb::dynamic_task_graph_node node(
      [job]() -> void {
        while (job->run()) {
        }
        delete job;
      }
    );

    taskGraphNodeRepository.add_dependencies( node, inDependencies );

    taskGraph.put( node );
  }
  else {
    ::oneapi::tbb::dynamic_task_graph_node& node = taskGraphNodeRepository.create_node(
      [job]() -> void {
        while (job->run()) {
        }
        delete job;
      },
      taskNumber
    );

    taskGraphNodeRepository.add_dependencies( taskNumber, inDependencies );

    taskGraph.put( node );
  }
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
      tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
    } else {
      delete myTask;
    }
  });
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  for (auto& p: inDependencies) {
    taskGraph.wait( taskGraphNodeRepository.get_node(p) );
  }
}


void tarch::multicore::native::waitForAllTasks() {
  taskGraph.wait_for_all();
}

#endif
