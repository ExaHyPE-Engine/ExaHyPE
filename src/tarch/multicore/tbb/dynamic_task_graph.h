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
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/concurrent_queue.h>


namespace tbb {
  class dynamic_task_graph_function_node;
  class dynamic_task_graph;
}


namespace tbb {
  using dynamic_task_graph_node_number = int;

  /**
   * Represents one node in task graph
   */
  class dynamic_task_graph_node {
    public:
      friend class dynamic_task_graph;

      static constexpr dynamic_task_graph_node_number AssignNewNumber = -1;

      /**
       */
      dynamic_task_graph_node(
        std::function<void()>           functor,
        dynamic_task_graph_node_number  dynamic_task_graph_node_number = AssignNewNumber
      );

      /**
       * Alternative wrapper around add_dependency( const dynamic_task_graph_node_number& node )
       */
      void add_dependency( const dynamic_task_graph_node& node );

      /**
       * Add a single dependency
       *
       * This works if and only if node has already been put into the task
       * graph. You cannot add any dependency before that (cmp concept of
       * anti-dependencies in OpenMP).
       */
      void add_dependency( const dynamic_task_graph_node_number& node );

      /**
       * Alternative wrapper around add_dependency( const dynamic_task_graph_node_number& node )
       */
      void add_dependencies( const std::set<dynamic_task_graph_node>& nodes );

      /**
       * Alternative wrapper around add_dependency( const dynamic_task_graph_node_number& node )
       */
      void add_dependencies( const std::set<dynamic_task_graph_node_number>& nodes );

      /**
       * Has task been submitted
       *
       * After you have submitted a node, you can use it as input dependency
       * for other nodes. However, you cannot add further dependencies to
       * itself anymore.
       */
      bool is_submitted() const;

      /**
       * Get task/node number
       *
       * Return the number assigned to the task. This is either the value
       * passed into the constructor through dynamic_task_graph_node_number, or
       * it is one assigned by the task graph if you have passed
       * AssignNewNumber.
       *
       * @return Node number identifying a unique task
       */
      dynamic_task_graph_node_number get_number() const;

    private:
      bool _submitted;
      std::vector<dynamic_task_graph_node_number>  _in_dependency;
      std::vector<dynamic_task_graph_node_number>  _out_dependency;
  };

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
        friend class dynamic_task_graph_node;

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
         * unless it has already been given one from your side.
         *
         * Once you have submitted a node, it becomes read-only, i.e. you
         * cannot add any further dependencies to it.
         */
        void put(dynamic_task_graph_node& node );

        /**
         * Wait for one specific node
         */
        void wait( const dynamic_task_graph_node& node );

        /**
         * Wait for one specific node
         */
        void wait( const dynamic_task_graph_node_number& node );

        /**
         * Wait for set of nodes
         */
        void wait( const std::set<dynamic_task_graph_node>& node );

        /**
         * Wait for set of nodes
         */
        void wait( const std::set<dynamic_task_graph_node_number>& node );

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
        /**
         * Spawned tasks where we hold
         */
//        oneapi::tbb::concurrent_hash_map<int, SpawnedTaskDescriptor*> spawnedTasks;

        /**
         * Queue of numbers that should be recycled later.
         */
        oneapi::tbb::concurrent_queue<dynamic_task_graph_node_number>  _recycle_numbers;
  };
}


//#endif


