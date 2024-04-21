// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. It
// supplements something to oneTBB which has been removed and which I need:
// dynamic tasking. Therefore I add it to the tbb namespace rather than
// Peano's tarch.
#pragma once


#include <functional>
#include <set>
#include <memory>


namespace tbb {
  class dynamic_task_graph_node;
  class dynamic_task_graph_spawned_node;
  class dynamic_task_graph;
}

namespace tbb {
  /**
   * Represents one node in task graph
   *
   */
  class dynamic_task_graph_node {
    public:
      friend class dynamic_task_graph;
      friend class dynamic_task_graph_spawned_node;

      /**
       * Create new task graph node
       *
       * Each task graph node is tied to a functor. When we create a task graph
       * node, the task is not yet submitted or ready. We have to explicitly
       * put() it into the task graph.
       */
      dynamic_task_graph_node( std::function<void()> functor );

      dynamic_task_graph_node( const dynamic_task_graph_node&  node );

      /**
       * Add a single dependency
       *
       * This works if and only if node has already been put into the task
       * graph. You cannot add any dependency before that (cmp concept of
       * anti-dependencies in OpenMP).
       */
      void add_dependency( const dynamic_task_graph_node& node );

      /**
       * Wrapper around add_dependency()
       */
      void add_dependencies( const std::set<dynamic_task_graph_node>& nodes );

      /**
       * Has task been submitted
       *
       * After you have submitted a node, you can use it as input dependency
       * for other nodes. However, you cannot add further dependencies to
       * itself anymore.
       */
      bool is_submitted() const;

    private:
      std::function<void()>                              _functor;

      std::shared_ptr<dynamic_task_graph_spawned_node>   _spawned_task;

      /**
       * In dependencies.
       *
       * Can only refer to submitted tasks. If a task is given a number and
       * it also depends on another task with the same number, this number
       * is not stored within this array. Instead, _depends_on_previous_task_with_same_number
       * is set.
       *
       * See dynamic_task_graph_spawned_node for a documentation why we have to
       * use shared pointers here.
       */
      std::vector< std::shared_ptr<dynamic_task_graph_spawned_node> >  _in_dependency;
  };
}

