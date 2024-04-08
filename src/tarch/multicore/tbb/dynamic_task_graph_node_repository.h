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
   * integer task numbers instead.
   */
  class dynamic_task_graph_node_repository {
    public:
      using dynamic_task_graph_node_number = int;

      dynamic_task_graph_node_repository();

      /**
       * Factor mechanism
       *
       * Gives you a new node. If there's already a task with that number,
       * then the factory mechanism adds in in-dependency to that one.
       *
       * The responsibility for this node resides within the repo.
       */
      dynamic_task_graph_node&  create_node(
        std::function<void()>           functor,
        dynamic_task_graph_node_number  number
      );

      dynamic_task_graph_node& get_node(const dynamic_task_graph_node_number& number);

      void add_dependency( dynamic_task_graph_node&  new_task_node, const dynamic_task_graph_node_number&  in_dependency);

      /**
       * Wrapper around multiple add_dependency() calls
       */
      void add_dependencies( dynamic_task_graph_node&  new_task_node, const std::set< dynamic_task_graph_node_number >&  in_dependencies );

    private:
      using HashMap = oneapi::tbb::concurrent_hash_map<dynamic_task_graph_node_number, dynamic_task_graph_node*>;
      HashMap  _node_container;
  };
}


//#endif

