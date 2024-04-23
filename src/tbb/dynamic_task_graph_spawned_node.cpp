#include "dynamic_task_graph_spawned_node.h"
#include "dynamic_task_graph.h"
#include "dynamic_task_graph_node.h"
#include "dynamic_task_graph_utils.h"

#include <iostream>




tbb::dynamic_task_graph_spawned_node::dynamic_task_graph_spawned_node(
  const dynamic_task_graph_node&  task_specification,
  dynamic_task_graph&             task_graph
):
  _state( State::Submitted ),
  _functor( task_specification._functor ),
  _number_of_in_dependencies(1),
  _task_graph(task_graph) {

  __TBB_TRACE_TASK_CREATE(this);
}


void tbb::dynamic_task_graph_spawned_node::incoming_task_has_terminated() {
  tbb::spin_mutex::scoped_lock lock(_semaphore);
  _number_of_in_dependencies--;

  __TBB_ASSERT( _number_of_in_dependencies>=0, "cannot wait for less than 0 other tasks" );

  lock.release();

  if (_number_of_in_dependencies==0) {
    run();
  }
}


void tbb::dynamic_task_graph_spawned_node::run() {
  __TBB_ASSERT( _number_of_in_dependencies==0, "can only run if no incoming dependencies anymore" );

  _state = State::Spawned;

  _task_graph._arena.enqueue([&] {
    __TBB_TRACE_TASK_RUN(this);

    _functor();

    _state = State::Complete;

    tbb::spin_mutex::scoped_lock lock(_semaphore);
    for (auto& p: _out_dependency) {
      p->incoming_task_has_terminated();
    }
    lock.release();

    __TBB_TRACE_TASK_COMPLETE(this);
  });
}
