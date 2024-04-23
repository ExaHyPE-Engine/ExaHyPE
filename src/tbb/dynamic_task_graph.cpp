#include "dynamic_task_graph.h"
#include "dynamic_task_graph_spawned_node.h"
#include "dynamic_task_graph_node.h"
#include "dynamic_task_graph_utils.h"


tbb::dynamic_task_graph::dynamic_task_graph(
  ::oneapi::tbb::task_arena&      arena
):
  _arena(arena) {
}


void tbb::dynamic_task_graph::put(dynamic_task_graph_node& node ) {
  node._spawned_task = std::make_shared<dynamic_task_graph_spawned_node>(node, *this);

  tbb::spin_mutex::scoped_lock   node_lock(node._spawned_task->_semaphore);
  SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex);

  for (auto& p: node._in_dependency) {
    if (
      _spawned_tasks.count(p)>0
      and
      p->_state!=dynamic_task_graph_spawned_node::State::Complete
    ) {
      tbb::spin_mutex::scoped_lock lock(p->_semaphore);
      p->_out_dependency.push_back( node._spawned_task );
      __TBB_TRACE_TASK_ADD_DEPENDENCY(p,node._spawned_task);
      node._spawned_task->_number_of_in_dependencies++;
    }
    else if (
      _spawned_tasks.count(p)>0
      and
      p->_state==dynamic_task_graph_spawned_node::State::Complete
    ) {
      _spawned_tasks.erase( p );
    }
  }

  _spawned_tasks.insert( node._spawned_task );

  node_lock.release();
  task_graph_lock.release();

  node._spawned_task->incoming_task_has_terminated();
}


void tbb::dynamic_task_graph::clean_up_completed_tasks() {
  SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex);

  auto p = _spawned_tasks.begin();
  while (p!=_spawned_tasks.end()) {
    if ((*p)->_state==dynamic_task_graph_spawned_node::State::Complete) {
      p = _spawned_tasks.erase(p);
    }
    else {
      p++;
    }
  }
}


void tbb::dynamic_task_graph::wait( const dynamic_task_graph_node& node ) {
  __TBB_ASSERT( node.is_submitted(), "can only wait for tasks that have been submitted" );

  __TBB_TRACE_TASK_WAIT( node._spawned_task );

  volatile bool task_still_in_queue = true;

  while ( task_still_in_queue ) {
    std::this_thread::yield();

    clean_up_completed_tasks();

    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex);
    task_still_in_queue = _spawned_tasks.count(node._spawned_task )>0
                      and node._spawned_task->_state!=dynamic_task_graph_spawned_node::State::Complete;
  }

  __TBB_TRACE_TASK_WAIT_COMPLETE( node._spawned_task );
}


void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node>& nodes ) {
  for (auto& p: nodes) {
    wait(p);
  }
}


void tbb::dynamic_task_graph::wait_for_all() {
  volatile bool task_still_in_queue = true;

  while ( task_still_in_queue ) {
    std::this_thread::yield();
    clean_up_completed_tasks();
    task_still_in_queue = not _spawned_tasks.empty();
  }
}


std::size_t tbb::dynamic_task_graph::size() const {
  return _spawned_tasks.size();
}
