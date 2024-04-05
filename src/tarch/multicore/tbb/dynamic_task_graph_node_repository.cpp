#include "dynamic_task_graph.h"


#include <cassert>
//#if defined(SharedTBB)

/*







tbb::dynamic_task_graph::dynamic_task_graph(
  ::oneapi::tbb::task_arena&      arena
):
  _arena(arena) {
}


void tbb::dynamic_task_graph::put(dynamic_task_graph_node& node ) {
  if (node._depends_on_previous_task_with_same_number) {
    // @todo
//    wait( node._number );
  }

  HashMap::accessor hash_map_accessor;

  if (node._number==dynamic_task_graph_node::AssignNewNumber) {
    bool is_new = false;
    dynamic_task_graph_node_number new_number = 0;
    while (is_new) {
      new_number++;
      is_new = _spawned_tasks.insert(hash_map_accessor, new_number);
    }
    node._number     = new_number;
  }
  else {
    bool is_new = _spawned_tasks.insert(hash_map_accessor, node._number);
    assert( is_new );
  }

  hash_map_accessor->second = nullptr;

}


void tbb::dynamic_task_graph::wait( const dynamic_task_graph_node& node ) {
  // @todo
}

void tbb::dynamic_task_graph::wait( const dynamic_task_graph_node_number& node ) {
  // @todo
}

void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node>& node ) {
  // @todo
}


void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node_number>& node ) {
  // @tood
}

void tbb::dynamic_task_graph::wait_for_all() {
  // @todo
}

std::size_t tbb::dynamic_task_graph::size() const {
  return _spawned_tasks.size();
}
*/



/**
 * Describes a task
 *
 * Describes a task which is pending or runnning, i.e. has been spawned. Each
 * such task knows
 *
 * - the actual task to be executed;
 * - how many in-dependencies it has;
 * - what the actual task does.
 *
 * If the number of in-dependencies becomes 0, the task is ready, i.e. we can
 * hand it over to TBB.
 * has its own mutex, so we can lock
 */
/*
struct SpawnedTaskDescriptor {
  tarch::multicore::Task* task;
  std::atomic<int>        numberOfPredecessorTasks;
  std::list<int>          successorTasks;
  std::mutex              mutex;
};
*/


/*
getArena(job)->enqueue([&, job] {
  while (job->run()) {
  }
  delete job;
});
*/

//#endif
