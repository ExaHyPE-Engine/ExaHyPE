#include "TaskHandleRepository.h"


#include <cassert>
#include "tarch/Assertions.h"



#if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
tarch::multicore::tbb::TaskHandleRepository::Handle& tarch::multicore::tbb::TaskHandleRepository::createNewTask() {

}
#else
#endif


void tarch::multicore::tbb::TaskHandleRepository::registerTask(
  Handle*      newTask,
  TaskNumber   number
) {
  HashMap::accessor hash_map_accessor;
  HashMap::accessor shadowed_hash_map_accessor;

  bool task_with_number_has_never_been_created_before  = _taskHandleContainer.insert(hash_map_accessor, number);

  if (task_with_number_has_never_been_created_before) {
    delete hash_map_accessor->second;
  }

  hash_map_accessor->second = newTask;
}


void tarch::multicore::tbb::TaskHandleRepository::addDependencies( Handle* newTask, const std::set< TaskNumber >&  in_dependencies ) {
  for (auto& p: in_dependencies) {
    addDependency( newTask, p );
  }
}


void tarch::multicore::tbb::TaskHandleRepository::addDependency( Handle* newTask, const TaskNumber&  number) {
  HashMap::accessor hash_map_accessor;
  if (_taskHandleContainer.find(hash_map_accessor, number)) {
    assertionMsg( hash_map_accessor->second!=nullptr, std::string( "predecessor task " + std::to_string(number) + " does not exist").c_str() );
    #if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW)
    newTask->add_predecessor( *(hash_map_accessor->second) );
    #else
    assertionMsg( hash_map_accessor->second->is_submitted(), std::string( "predecessor task " + std::to_string(number) + " is not yet submitted").c_str() );
    newTask->add_dependency( *(hash_map_accessor->second) );
    #endif
  }
}


tarch::multicore::tbb::TaskHandleRepository::~TaskHandleRepository() {
  for (auto& pair : _taskHandleContainer) {
    delete pair.second;
  }
}
