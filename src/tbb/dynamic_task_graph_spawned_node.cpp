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
  _task_graph(task_graph)
  #if TBB_USE_DEBUG>0
  , _task_description(task_specification._task_description)
  #endif
{
  __TBB_TRACE_TASK_CREATE(this);
}


void tbb::dynamic_task_graph_spawned_node::add_incoming_task_dependency() {
  tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);
  _number_of_in_dependencies++;
}


void tbb::dynamic_task_graph_spawned_node::incoming_task_has_terminated() {
  tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);
  _number_of_in_dependencies--;
  __TBB_ASSERT( _number_of_in_dependencies>=0, "cannot wait for less than 0 other tasks" );
  const bool is_ready = _number_of_in_dependencies==0;
  lock.release();

  if (is_ready) {
    run();
  }
}


void tbb::dynamic_task_graph_spawned_node::run() {
  __TBB_ASSERT( _number_of_in_dependencies==0, "can only run if no incoming dependencies anymore" );

  _state = State::Spawned;

  _task_graph._task_group.run([&] {

    __TBB_TRACE_TASK_RUN(this);

    _functor();

    tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);
    for (auto& p: _out_dependency) {
      p->incoming_task_has_terminated();
    }
    _state = State::Complete;

    __TBB_TRACE_TASK_COMPLETE(this);
  });

  __TBB_ASSERT( _number_of_in_dependencies==0, "somebody has decreased the dependency counter once again" );
}



bool tbb::dynamic_task_graph_spawned_node::has_terminated() const {
  return _state==State::Complete;
}


bool tbb::dynamic_task_graph_spawned_node::add_successor( std::shared_ptr<dynamic_task_graph_spawned_node> successor ) {
  bool result = false;
  tbb::spin_mutex::scoped_lock lock(_dependencies_semaphore);

  if ( _state != State::Complete ) {
    _out_dependency.push_back( successor );
    result = true;
  }

  return result;
}


std::string tbb::dynamic_task_graph_spawned_node::to_string() const {
  std::string result;

  result = "(";
  switch (_state) {
    case State::Complete:
      result += "complete";
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

  #if TBB_USE_DEBUG>0
  result += "," + _task_description;
  #endif

  result += ")";

  return result;
}
