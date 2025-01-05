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
  _number_of_in_dependencies--;

  __TBB_ASSERT( _number_of_in_dependencies>=0, "cannot wait for less than 0 other tasks" );
  
  const bool is_ready = _number_of_in_dependencies==0;

  if (is_ready) {
    run();
  }
}


void tbb::dynamic_task_graph_spawned_node::run() {
  __TBB_ASSERT( _number_of_in_dependencies==0, "can only run if no incoming dependencies anymore" );

  _state = State::Spawned;

  _task_graph._arena.enqueue([&] {
    __TBB_TRACE_TASK_RUN(this);

    _functor();

    _state = State::Terminated;

    tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);
    for (auto& p: _out_dependency) {
      p->incoming_task_has_terminated();
    }
    _state = State::NotifiedSuccessors;
    lock.release();

    _state.notify_all();
    __TBB_TRACE_TASK_COMPLETE(this);
  });
}



bool tbb::dynamic_task_graph_spawned_node::add_successor( std::shared_ptr<dynamic_task_graph_spawned_node> successor ) {
  bool result = false;
  tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);

  if (
    _state != State::Terminated
    and
    _state != State::NotifiedSuccessors
  ) {
    _out_dependency.push_back( successor );
    result = true;
  }

  return result;
}


std::string tbb::dynamic_task_graph_spawned_node::to_string() const {
  std::string result;

  result = "(";
  switch (_state) {
    case State::Terminated:
      result += "terminated";
      break;
    case State::NotifiedSuccessors:
      result += "notified-successors";
      break;
    case State::Spawned:
      result += "spawned";
      break;
    case State::Submitted:
      result += "submitted";
      break;
  }

  result += ",#in=" + std::to_string( _number_of_in_dependencies );
  result += ",#out=" + std::to_string( _out_dependency.size() );
  result += ")";

  return result;
}
