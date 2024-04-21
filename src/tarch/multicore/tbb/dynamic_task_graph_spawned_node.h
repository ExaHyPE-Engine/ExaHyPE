// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. It
// supplements something to oneTBB which has been removed and which I need:
// dynamic tasking. Therefore I add it to the tbb namespace rather than
// Peano's tarch.
#pragma once


#include <functional>
#include <set>
#include <tbb/task_arena.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/spin_mutex.h>


namespace tbb {
  class dynamic_task_graph_node;
  class dynamic_task_graph_spawned_node;
  class dynamic_task_graph;
}


namespace tbb {
  /**
   * Spawned node
   *
   * This class is only used internally within the task graph lib and not to be
   * used directly by users. Users create instances of dynamic_task_graph_node.
   * Once they put this node into a task_graph instance, it is tied to a new
   * instance of spawned_task_graph_node.
   *
   *
   * ## Memory/heap management
   *
   * As long as a task graph is alive, users might use it to introduce new task
   * dependencies. So we have to ensure that the memory location is absolutely
   * unique. Notably, we may not encounter a situation where a user submits a
   * task A, this task is processed immediately, then creates a task B and a
   * task C which depends on A (but not B). So B may never ever end up at the
   * same memory location as A.
   *
   * To avoid this, we work exclusively with smart pointers referencing spawned
   * nodes.
   */
  class dynamic_task_graph_spawned_node {
    public:
      friend class dynamic_task_graph;

      enum class State {
        Submitted,
        Spawned,
        Complete
      };

      /**
       * Create a new task node in the graph
       *
       * Creating a new task node means that we have to "invert" the edges in
       * the underlying graph: An instance of task_specification knows which
       * task are incoming. For the actual task graph, we need the outgoing
       * tasks.
       *
       * The default spawned task has one incoming dependency, i.e. is not
       * ready. You have to manually decrease the counter after construction
       * once all dependencies are in place.
       */
      dynamic_task_graph_spawned_node(
        const dynamic_task_graph_node&  task_specification,
        dynamic_task_graph&             task_graph
      );

      /**
       * You cannot copy a spawned node
       */
      dynamic_task_graph_spawned_node(const dynamic_task_graph_spawned_node& ) = delete;

      void incoming_task_has_terminated();

      /**
       * Run the task
       *
       * This is the actual routine which hands the task over to TBB's runtime.
       * The system is allowed to invoke it if and only if the task is ready,
       * i.e. has no pending incoming dependencies anymore.
       *
       *
       * ## Algorithm steps
       *
       * 1. Invoke the functor, i.e. do the actual task calculations. Note that
       *    other tasks still might add further outgoing dependencies at this
       *    point.
       * 2. Create our own smart pointer to the underlying instance of
       *    dynamic_tsak_graph_spawned_node. We will remove it from the list of
       *    spawned tasks next, which means it could be destroyed at this point.
       *    We still have to tidy it up however, so better to hold our own
       *    smart pointer.
       *
       *
       * 2. Remove the task from the set of spawned tasks. Noone should use this
       *    task after that anymore.
       * 3. Lock the task, as we now will change things. In theory, this should
       *    not be necessary, but someone might still hold a pointer to this
       *    task and try to alter it while we sort out the outgoing dependencies
       *    and ramp up the task.
       * 4. Inform all outgoing tasks that we are done.
       *
       *
       * ## Implementation
       *
       * We work with catch by reference here, as we know that all the task's
       * fields are still alive while we execute the task.
       *
       * @todo Write something about lazy deletion
       */
      void run();
    private:
      std::atomic<State>                              _state;
      std::function<void()>                           _functor;
      tbb::spin_mutex                                 _semaphore;
      dynamic_task_graph&                             _task_graph;
      int                                             _number_of_in_dependencies;
      std::vector< std::shared_ptr<dynamic_task_graph_spawned_node> >   _out_dependency;
  };

}
