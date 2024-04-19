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

  tbb::spin_mutex::scoped_lock lock(node._spawned_task->_semaphore);

  for (auto& p: node._in_dependency) {
    // Does the task still exist?
    if ( _spawned_tasks.count(p.get())>0 ) {
      tbb::spin_mutex::scoped_lock lock(p->_semaphore);
      p->_out_dependency.push_back( node._spawned_task );
      __TBB_TRACE_TASK_ADD_DEPENDENCY(p,node._spawned_task);
      node._spawned_task->_number_of_in_dependencies++;
    }
  }

  _spawned_tasks.insert( node._spawned_task.get() );

  lock.release();

  node._spawned_task->incoming_task_has_terminated();
}



void tbb::dynamic_task_graph::wait( const dynamic_task_graph_node& node ) {
  __TBB_ASSERT( node.is_submitted(), "can only wait for tasks that have been submitted" );

  __TBB_TRACE_TASK_WAIT( node._spawned_task );

  while ( _spawned_tasks.count(node._spawned_task.get() )>0 ) {
    std::this_thread::yield();
  }
}


void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node>& nodes ) {
  for (auto& p: nodes) {
    wait(p);
  }
}


void tbb::dynamic_task_graph::wait_for_all() {
  while ( not _spawned_tasks.empty() ) {
    std::this_thread::yield();
  }
}


std::size_t tbb::dynamic_task_graph::size() const {
  return _spawned_tasks.size();
}
