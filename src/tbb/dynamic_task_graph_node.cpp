#include "dynamic_task_graph_node.h"

#include "dynamic_task_graph_utils.h"
#include "dynamic_task_graph_spawned_node.h"
#include <oneapi/tbb/spin_mutex.h>



tbb::dynamic_task_graph_node::dynamic_task_graph_node(
  std::function<void()>           functor
):
  _functor(functor),
  _spawned_task(nullptr),
  _in_dependency() {
}


tbb::dynamic_task_graph_node::dynamic_task_graph_node(
  std::function<void()> functor,
  const std::string& task_description
):
  _functor(functor),
  _spawned_task(nullptr),
  _in_dependency()
  #if TBB_USE_DEBUG>0
  , _task_description(task_description)
  #endif
{}


tbb::dynamic_task_graph_node::dynamic_task_graph_node( const dynamic_task_graph_node&  other ):
  _functor(other._functor),
  _spawned_task(other._spawned_task),
  _in_dependency(other._in_dependency) {
  __TBB_ASSERT( other.is_submitted(), "never copy a task node that is not yet submitted" );
  #if TBB_USE_DEBUG>0
  _task_description = other._task_description;
  #endif
}


void tbb::dynamic_task_graph_node::add_dependency( const dynamic_task_graph_node& node ) {
  __TBB_ASSERT( not is_submitted(), "cannot add dependencies to node that is already submitted" );
  __TBB_ASSERT( node.is_submitted(), "can only add dependencies to nodes that are already submitted" );
  _in_dependency.push_back( node._spawned_task );
}


void tbb::dynamic_task_graph_node::add_dependencies( const std::set<dynamic_task_graph_node>& nodes ) {
  for (auto& p: nodes) {
    add_dependency(p);
  }
}


bool tbb::dynamic_task_graph_node::is_submitted() const {
  return _spawned_task!=nullptr;
}


std::string tbb::dynamic_task_graph_node::to_string() const {
  return "(hosted-task=" + _spawned_task->to_string() + ",#in-deps=" + std::to_string(_in_dependency.size() ) + ")";
}
