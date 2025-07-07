#include "dynamic_task_graph.h"
#include "dynamic_task_graph_spawned_node.h"
#include "dynamic_task_graph_node.h"
#include "dynamic_task_graph_utils.h"


tbb::dynamic_task_graph::dynamic_task_graph(
  ::oneapi::tbb::task_group&      group
):
  _task_group(group) {
}

#include <iostream>

void tbb::dynamic_task_graph::put(dynamic_task_graph_node& node ) {
  node._spawned_task = std::make_shared<dynamic_task_graph_spawned_node>(node, *this);

  {
    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex, true); // is writer
    _spawned_tasks.insert( node._spawned_task );
  }

  bool created_at_least_one_new_dependency = false;
  {
    SpawnedTaskMutex::scoped_lock  task_graph_lock(_spawned_task_mutex, false); // is writer

    for (auto& p: node._in_dependency) {
      node._spawned_task->add_incoming_task_dependency();
      if (
        _spawned_tasks.count(p)>0
        and
        p->add_successor(node._spawned_task)
      ) {
        __TBB_TRACE_TASK_ADD_DEPENDENCY(p,node._spawned_task);
        created_at_least_one_new_dependency = true;
      }
      else {
        node._spawned_task->incoming_task_has_terminated();
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
    if ((*p)->has_terminated()) {
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
       and not node._spawned_task->has_terminated();
  };

  if (isTaskStillInQueue()) {
    //_task_group.wait();
    //std::this_thread::yield();
    clean_up_completed_tasks();
  }

  __TBB_TRACE_TASK_WAIT_COMPLETE( node._spawned_task );
}


void tbb::dynamic_task_graph::wait( const std::set<dynamic_task_graph_node>& nodes ) {
  for (auto p: nodes) {
    wait(p);
  }
}


void tbb::dynamic_task_graph::wait_for_all() {
  _task_group.wait();
  clean_up_completed_tasks();

  while (not _spawned_tasks.empty()) {
    _task_group.wait();
    clean_up_completed_tasks();

//    // @todo Analysis: There are some tasks that are left-over, i.e. are not released
//    if (_spawned_tasks.size()<=2 and not _spawned_tasks.empty()) {
      std::cout << std::endl << "#size=" << _spawned_tasks.size();
      for (auto& p: _spawned_tasks)
        std::cout << ", " << p->to_string();
      std::cout.flush();
//      __TBB_ASSERT( false, "should not happen" );
//    }
  }

  __TBB_ASSERT( _spawned_tasks.empty(), "set of spawned tasks should be empty" );
}


std::size_t tbb::dynamic_task_graph::size() const {
  return _spawned_tasks.size();
}
