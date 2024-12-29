// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org.
#pragma once


#include "config.h"


#include <functional>
#include <set>
#include <tbb/task_arena.h>

#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/concurrent_queue.h>


#if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
// In the new TBB version, all the handle and dynamic task graph stuff is
// merged into the core tasking. No additional includes required.
#else
#include "tbb/dynamic_task_graph.h"
#include "tbb/dynamic_task_graph_node.h"
#include "tbb/dynamic_task_graph_utils.h"
#endif


namespace tarch {
  namespace multicore {
    namespace tbb {
      class TaskHandleRepository;
    }
  }
}

/**
 * Simple utility class around dynamic task graphs to work with integer task numbers
 *
 * It handles the management of the nodes, such that users can work with
 * integer task numbers instead. The only interesting thing here is that
 * users can obviously "overwrite" tasks, i.e. create a new task with the
 * same number over and over again. Usually, the newest task is the ruling
 * one, i.e. hides all previous ones. However, when you add a task dependency
 * for a task to itself, this bookkeeping interprets this as: Hey, I have a
 * task with number A and now create a new task with number A, but that one
 * should only run once the first one has completed.
 *
 * I use the name handle here, as we never directly interact with tasks. In
 * TBB, there's actually the notion of a task handle. In my own TBB extension,
 * we work with a dynamic task graph node which wraps (contains) a task that
 * is shielded away. You would never interact with the tasks directly here
 * either.
 */
class tarch::multicore::tbb::TaskHandleRepository {
  public:
    using TaskNumber = int;

    TaskHandleRepository() = default;
    ~TaskHandleRepository();

    #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
    using Handle = ::oneapi::tbb::task_handle;
    #else
    using Handle = ::tbb::dynamic_task_graph_node;
    #endif



    /**
     * Factory mechanism
     *
     * If there's already a task with that number, it will be thrown away.
     * If you want to have tasks with in/out dependencies (identified
     * through the same task number), you have to add these a priori.
     */
    void registerTask(
      Handle*      newTask,
      TaskNumber   number
    );

    /**
     * Add dependency
     *
     * If in_dependency equals new_task_node, then we assume that it refers
     * to a previously submitted task with the same number. If no such task
     * is known, the add_dependency creates an anti-dependency. We simply
     * ignore it.
     */
    void addDependency( Handle*      newTask, const TaskNumber&  in_dependency);

    void addDependencies( Handle*    newTask, const std::set< TaskNumber >&  in_dependencies );

  private:
    #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
    using HashMap = oneapi::tbb::concurrent_hash_map<TaskNumber, oneapi::tbb::task_handle*>;
    #else
    using HashMap = oneapi::tbb::concurrent_hash_map<TaskNumber, ::tbb::dynamic_task_graph_node*>;
    #endif

    HashMap  _taskHandleContainer;
};
