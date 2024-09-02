#if defined(TBB_USE_TASK_GROUP_PREVIEW)

#include "task_handle_repository.h"
#include <cassert>


tbb::task_handle_repository::task_handle_repository()
    : _handle_container(), _shadowed_handle_container() {}


oneapi::tbb::task_handle& tbb::task_handle_repository::include_handle(
  oneapi::tbb::task_handle&& handle,
  task_handle_number         number
) {
  HashMap::accessor hash_map_accessor;
  HashMap::accessor shadowed_hash_map_accessor;

  bool task_with_number_has_never_been_created_before  = _handle_container.insert(hash_map_accessor, number);

  if (task_with_number_has_never_been_created_before) {
    hash_map_accessor->second          = nullptr;
  }
  else {
    bool task_with_number_has_never_been_shadowed_before = _shadowed_handle_container.insert(shadowed_hash_map_accessor, number);

    if (not task_with_number_has_never_been_shadowed_before) {
      delete shadowed_hash_map_accessor->second;
    }

    shadowed_hash_map_accessor->second = hash_map_accessor->second;
    hash_map_accessor->second          = nullptr;
  }

  hash_map_accessor->second = new task_handle(std::move(handle));

  //__TBB_TRACE_TASK_MEMORIZE_NODE(number,hash_map_accessor->second);
  return *(hash_map_accessor->second);
}


oneapi::tbb::task_handle& tbb::task_handle_repository::get_node(const task_handle_number& number) {
  HashMap::accessor hash_map_accessor;
  __TBB_ASSERT( _handle_container.count(number)==1, std::string( "handle " + std::to_string(number) + " does not exist").c_str() );
  _handle_container.find(hash_map_accessor,number);
  return *(hash_map_accessor->second);
}


void tbb::task_handle_repository::add_dependency( const task_handle_number& new_task_node, const task_handle_number&  number) {
  if (new_task_node==number) {
    HashMap::accessor hash_map_accessor_for_previous_task_with_same_number;

    bool theres_a_previous_task_with_this_number = _shadowed_handle_container.find(hash_map_accessor_for_previous_task_with_same_number, number);

    if (theres_a_previous_task_with_this_number) {
      __TBB_ASSERT( hash_map_accessor_for_previous_task_with_same_number->second!=nullptr, std::string( "no backup of predecessor task " + std::to_string(number) + " available").c_str() );
      get_node(new_task_node).add_predecessor( *(hash_map_accessor_for_previous_task_with_same_number->second) );
    }
  }
  else {
    add_dependency( get_node(new_task_node), number );
  }
}


void tbb::task_handle_repository::add_dependencies( const task_handle_number& new_task_node, const std::set< task_handle_number >&  in_dependencies ) {
  for (auto& p: in_dependencies) {
    add_dependency( new_task_node, p );
  }
}


void tbb::task_handle_repository::add_dependency( oneapi::tbb::task_handle&  new_task_node, const task_handle_number&  number) {
  HashMap::accessor hash_map_accessor;
  if (_handle_container.find(hash_map_accessor, number)) {
    __TBB_ASSERT( hash_map_accessor->second!=nullptr, std::string( "predecessor task " + std::to_string(number) + " does not exist").c_str() );
    //__TBB_ASSERT( hash_map_accessor->second->is_submitted(), std::string( "predecessor task " + std::to_string(number) + " is not yet submitted").c_str() );
    new_task_node.add_predecessor( *(hash_map_accessor->second) );
  }
}


void tbb::task_handle_repository::add_dependencies( oneapi::tbb::task_handle&  new_task_node, const std::set< task_handle_number >&  in_dependencies ) {
  for (auto& p: in_dependencies) {
    add_dependency( new_task_node, p );
  }
}

tbb::task_handle_repository::~task_handle_repository() {
  for (auto& pair : _handle_container) {
    delete pair.second;
  }
}

#endif