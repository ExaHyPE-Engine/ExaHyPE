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

  class spawned_task_graph_node: public dynamic_task_graph_node {
    private:
      std::mutex  _semaphore;
      std::vector<spawned_task_graph_node*>  _out_dependency;
  };

}
