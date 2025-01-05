#include "dynamic_task_graph.h"
#include "dynamic_task_graph_spawned_node.h"
#include "dynamic_task_graph_node.h"
#include "dynamic_task_graph_utils.h"


tbb::dynamic_task_graph::dynamic_task_graph(
  ::oneapi::tbb::task_arena&      arena
):
  _arena(arena) {
}

#include <iostream>

void tbb::dynamic_task_graph::put(dynamic_task_graph_node& node ) {
  node._spawned_task = std::make_shared<dynamic_task_graph_spawned_node>(node, *this);

  {
    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex, true); // is writer
    __TBB_ASSERT( node._spawned_task->_number_of_in_dependencies>=1, "invalid number of in-dependencies (there has to be at least one left over)" );
    _spawned_tasks.insert( node._spawned_task );
  }


  bool created_at_least_one_new_dependency = false;
  {
    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex, false); // is writer

    for (auto& p: node._in_dependency) {
      // @todo: Docu muss ich erst erhoehen, weil sonst kann es negativ reinlaufen wenn add_successor ge
      node._spawned_task->_number_of_in_dependencies++;
      if (
        _spawned_tasks.count(p)>0
        and
        p->add_successor(node._spawned_task)
      ) {
        __TBB_TRACE_TASK_ADD_DEPENDENCY(p,node._spawned_task);
        created_at_least_one_new_dependency = true;
      }
      else {
        node._spawned_task->_number_of_in_dependencies--;
      }
    }
  }

  node._spawned_task->incoming_task_has_terminated();

  if (not created_at_least_one_new_dependency and not node._in_dependency.empty()) {
    clean_up_completed_tasks();
  }
}


void tbb::dynamic_task_graph::clean_up_completed_tasks() {
  SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex, true); // is writer

  auto p = _spawned_tasks.begin();
  while (p!=_spawned_tasks.end()) {
    if ((*p)->_state==dynamic_task_graph_spawned_node::State::NotifiedSuccessors) {
      p = _spawned_tasks.erase(p);
    }
    else {
      p++;
    }
  }
}


void tbb::dynamic_task_graph::wait( dynamic_task_graph_node node ) {
  __TBB_ASSERT( node.is_submitted(), "can only wait for tasks that have been submitted" );

  __TBB_TRACE_TASK_WAIT( node._spawned_task );

  auto isTaskStillInQueue = [&]() -> bool {
    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex);
    __TBB_ASSERT( _spawned_tasks.count(node._spawned_task )<=1, "several tasks in lookup table" );

    return _spawned_tasks.count(node._spawned_task )>0
       and node._spawned_task->_state!=dynamic_task_graph_spawned_node::State::Terminated
       and node._spawned_task->_state!=dynamic_task_graph_spawned_node::State::NotifiedSuccessors;
  };

  if (isTaskStillInQueue()) {
    clean_up_completed_tasks();

    dynamic_task_graph_spawned_node::State old_state = node._spawned_task->_state;
    while (isTaskStillInQueue()) {
      node._spawned_task->_state.wait( old_state );
      arena drain; or ask group
    }
  }

  __TBB_TRACE_TASK_WAIT_COMPLETE( node._spawned_task );
}


void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node>& nodes ) {
  for (auto p: nodes) {
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
