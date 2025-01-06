// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Task.h"
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
#include <atomic>


#include "TaskHandleRepository.h"



namespace {
  /**
   * Helper class for dealing with a global task_group
   * 
   * A task_group expects a call to wait before the end of its lifetime,
   * which is not expected by tarch's tasking interface. Thus, a
   * special class is needed that waits in its destructor, such that
   * there is always a call to wait.
   */
  class GlobalTaskGroup : public ::oneapi::tbb::task_group {
    public:
      GlobalTaskGroup() : task_group() {}
      ~GlobalTaskGroup() {
        wait();
      }
  };
 
  /**
   * used to store task_handles and manage dependencies
   */
  ::tarch::multicore::tbb::TaskHandleRepository taskHandleRepository;
  
  /**
   * Global task group used to execute the majority of tasks
   * 
   * TBB's improved task_group api allows the user to build up dynamic dependencies between tasks in a task_grup,
   * which is heavily used in this implementation of tarch's tasking interface.
   */
  GlobalTaskGroup     globalTaskGroup;

  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
  #else
  static ::oneapi::tbb::dynamic_task_graph                  taskGraph(globalTaskGroup);
  #endif
}


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
  globalTaskGroup.run([&, myTask, copyOfTasksOfSameType] {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
    } else {
      delete myTask;
    }
  });
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
   ::oneapi::tbb::task_handle*  handle = new ::oneapi::tbb::task_handle(
     globalTaskGroup.defer(
       []() -> void {
       }
     )
   );
  #else
   ::tbb::dynamic_task_graph_node* handle = new ::tbb::dynamic_task_graph_node(
     []() -> void {
     }
   );
  #endif

  taskHandleRepository.addDependencies( handle, inDependencies );

  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
  //globalTaskGroup.run( std::move(*handle) );
  globalTaskGroup.run_and_wait( handle );
  #else
  taskGraph.put( *handle );
  taskGraph.wait( *handle );
  #endif

  delete handle;
}


void tarch::multicore::native::waitForAllTasks() {
  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
  #else
  taskGraph.wait_for_all();
  #endif

  globalTaskGroup.wait();
}


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
  ::oneapi::tbb::task_handle*  handle = new ::oneapi::tbb::task_handle(
      globalTaskGroup.defer(
        [job]() -> void {
          while (job->run()) {
          }
          delete job;
        }
      )
    );
  #else
    ::tbb::dynamic_task_graph_node* handle = new ::tbb::dynamic_task_graph_node(
      [job]() -> void {
        while (job->run()) {
        }
        delete job;
      }
    );
  #endif

  taskHandleRepository.addDependency( handle, taskNumber );
  taskHandleRepository.addDependencies( handle, inDependencies );

  #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
  //globalTaskGroup.run( std::move(*handle) );
  globalTaskGroup.run( handle );
  #else
  taskGraph.put( *handle );
  #endif

  if (taskNumber==NoOutDependencies) {
    delete handle;
  }
  else {
    taskHandleRepository.registerTask(handle, taskNumber);
  }
}


#endif // defined(SharedTBB)
