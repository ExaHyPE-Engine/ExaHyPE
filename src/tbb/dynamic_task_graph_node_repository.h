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
#include <oneapi/tbb/concurrent_queue.h>


namespace tbb {
  class dynamic_task_graph_node;
  class dynamic_task_graph_spawned_node;
  class dynamic_task_graph;
  class dynamic_task_graph_node_repository;
}

namespace tbb {
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
   */
  class dynamic_task_graph_node_repository {
    public:
      using dynamic_task_graph_node_number = int;

      dynamic_task_graph_node_repository();

      /**
       * Factor mechanism
       *
       * Gives you a new node. If there's already a task with that number,
       * then this task is backed up in the data structure 
       * _shadowed_node_container. This way, we can add in-out dependencies
       * (the task with the same number is an in-dependency for the newly
       * created task). To add this dependency is the job of the user, i.e.
       * this routine just ensures that the backup/shadow container is 
       * properly befilled.
       */
      dynamic_task_graph_node&  create_node(
        std::function<void()>           functor,
        dynamic_task_graph_node_number  number
      );

      dynamic_task_graph_node& get_node(const dynamic_task_graph_node_number& number);

      /**
       * Alternative to number-based routine
       *
       * This routine is slightly less powerful, as you cannot create any links
       * to previous tasks with the same number: By the time you have created a
       * task with a number that you pipe in here with new_task_node, you have
       * no access to previous tasks which did carry the same number.
       */
      void add_dependency( dynamic_task_graph_node&  new_task_node, const dynamic_task_graph_node_number&  in_dependency);

      /**
       * Add dependency
       *
       * If in_dependency equals new_task_node, then we assume that it refers
       * to a previously submitted task with the same number. If no such task
       * is known, the add_dependency creates an anti-dependency. We simply
       * ignore it.
       */
      void add_dependency( const dynamic_task_graph_node_number&  new_task_node, const dynamic_task_graph_node_number&  in_dependency);

      /**
       * Wrapper around multiple add_dependency() calls
       */
      void add_dependencies( dynamic_task_graph_node&  new_task_node, const std::set< dynamic_task_graph_node_number >&  in_dependencies );
      void add_dependencies( const dynamic_task_graph_node_number&  new_task_node, const std::set< dynamic_task_graph_node_number >&  in_dependencies );

    private:
      using HashMap = oneapi::tbb::concurrent_hash_map<dynamic_task_graph_node_number, dynamic_task_graph_node*>;
      HashMap  _node_container;

      /**
       * Holds those nodes which have been reused already.
       */
      HashMap  _shadowed_node_container;
  };
}


