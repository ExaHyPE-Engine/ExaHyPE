// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. It
// supplements something to oneTBB which has been removed and which I need:
// dynamic tasking. Therefore I add it to the tbb namespace rather than
// Peano's tarch.
#pragma once

//#if defined(SharedTBB)


#include <functional>
#include <set>
#include <tbb/task_arena.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/concurrent_unordered_set.h>


namespace tbb {
  class dynamic_task_graph_node;
  class dynamic_task_graph_spawned_node;
  class dynamic_task_graph;
  class dynamic_task_graph_node_repository;
}


/**
 * @page tbb_dynamic_task_graph_design TBB dynamic task graph wrapper
 *
 * Users build up the task graph by creating instances of task graph
 * node objects. Once we submit a node to the task graph, it is given a
 * link to a task graph node on the heap and becomes itself invariant,
 * i.e. you cannot add further dependencies anymore. Before that, each
 * node can be added links to previously submitted task graph nodes.
 *
 * It is important to hold submitted task graphs on the heap, as they need
 * to host a mutex. The task graph also holds a set of all heap objects
 * which are still alive, as a user might refer to a previously generated
 * task which is already complete and hence removed. That is: incoming
 * dependencies might "point" to tasks which don't exist anymore. Later
 * on, outgoing dependencies always point to something valid.
 *
 * ## Usage
 *
 * Users never interact with spawned nodes.
 */
namespace tbb {
  /**
   * Very simple dynamic task graph interface
   *
   * Each task graph is tied to an arena and maintains a set of graphs nodes
   * aka tasks with dependencies between them. Immediately after tasks become
   * ready, the task graph spawns them into the arena.
   *
   * ## Implementation
   *
   * The task graph holds a big hash map mapping individual task numbers of
   * type dynamic_task_graph_node_number onto dynamic_task_graph_node which
   * in turn hold the actual task graph. The entries are added to the list
   * when we construct a node. They are removed after the ask has terminated.
   *
   * @author Tobias Weinzierl
   */
  class dynamic_task_graph {
      public:
        friend class dynamic_task_graph_spawned_node;

        /**
         * Construct task graph over arena
         *
         * @param arena Arena to be used once the task becomes ready. It is the
         *   user's responsibility to ensure that the arena is valid.
         */
        dynamic_task_graph(
          ::oneapi::tbb::task_arena&      arena
        );

        /**
         * Put a new node into the task graph
         *
         * The ownership of the node is passed over to the task graph. You
         * don't have to delete it yourself. This is different to oneAPI's
         * flow graph, where we can work with references, as we rely on the
         * fact that nodes stay alive until we call wait_for_all. Here,
         * everything is dynamic and the task might or might not be executed
         * immediately or be referenced later on.
         *
         * The routine will assign node a valid dynamic_task_graph_node_number,
         * unless it has already been given one from your side. It is always
         * faster if you have assigned a task a number already, as we then
         * don't have to search for a new task number.
         *
         * Once you have submitted a node, it becomes read-only, i.e. you
         * cannot add any further dependencies to it.
         *
         *
         * ## Implementation
         *
         * 1. If the task node depends upon a task with the same number,
         *    memorise this additional task. We'll remove it from the hash
         *    map next, so we have to keep track of its memory location.
         */
        void put(dynamic_task_graph_node& node );

        /**
         * Wait for one specific node
         */
        void wait( const dynamic_task_graph_node& node );

        /**
         * Wait for set of nodes
         */
        void wait( const std::set<dynamic_task_graph_node>& node );

        /**
         * Wait for all tasks to finish
         *
         * This drains the graph, i.e. size()==0 will hold once the function
         * terminates.
         */
        void wait_for_all();

        /**
         * Number of tasks in graph
         *
         * This includes all running and pending tasks.
         */
        std::size_t size() const;
      private:
        ::oneapi::tbb::task_arena&      _arena;

        /**
         * See documentation of dynamic_task_graph_spawned_node for a
         * discussion why we have to use smart pointers here.
         */
        using SpawnedTasksContainer = oneapi::tbb::concurrent_unordered_set< dynamic_task_graph_spawned_node* >;

        /**
         * Spawned tasks known to system
         */
        SpawnedTasksContainer _spawned_tasks;
  };
}


//#endif

