#include "dynamic_task_graph.h"
#include "dynamic_task_graph_node.h"
#include "dynamic_task_graph_node_repository.h"
#include "dynamic_task_graph_utils.h"


#include <cassert>


tbb::dynamic_task_graph_node_repository::dynamic_task_graph_node_repository() {
}


tbb::dynamic_task_graph_node&  tbb::dynamic_task_graph_node_repository::create_node(
  std::function<void()>           functor,
  dynamic_task_graph_node_number  number
) {
  HashMap::accessor hash_map_accessor;

  bool is_new = _node_container.insert(hash_map_accessor, number);

  if (not is_new) {
    delete hash_map_accessor->second;
  }

  hash_map_accessor->second = new dynamic_task_graph_node( functor );

  __TBB_TRACE_TASK_MEMORIZE_NODE(number,hash_map_accessor->second);

  return *(hash_map_accessor->second);
}


tbb::dynamic_task_graph_node& tbb::dynamic_task_graph_node_repository::get_node(const dynamic_task_graph_node_number& number) {
  HashMap::accessor hash_map_accessor;
  __TBB_ASSERT( _node_container.count(number)==1, std::string( "node " + std::to_string(number) + " does not exist").c_str() );
  _node_container.find(hash_map_accessor,number);
  return *(hash_map_accessor->second);
}


void tbb::dynamic_task_graph_node_repository::add_dependency( dynamic_task_graph_node&  new_task_node, const dynamic_task_graph_node_number&  number) {
  HashMap::accessor hash_map_accessor;
  if (_node_container.find(hash_map_accessor, number)) {
    __TBB_ASSERT( hash_map_accessor->second!=nullptr, std::string( "predecessor task " + std::to_string(number) + " does not exist").c_str() );
    __TBB_ASSERT( hash_map_accessor->second->is_submitted(), std::string( "predecessor task " + std::to_string(number) + " is not yet submitted").c_str() );
    new_task_node.add_dependency( *(hash_map_accessor->second) );
  }
}


void tbb::dynamic_task_graph_node_repository::add_dependencies( dynamic_task_graph_node&  new_task_node, const std::set< dynamic_task_graph_node_number >&  in_dependencies ) {
  for (auto& p: in_dependencies) {
    add_dependency( new_task_node, p );
  }
}

