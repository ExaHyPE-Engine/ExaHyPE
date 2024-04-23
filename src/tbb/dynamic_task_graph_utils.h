#pragma once


/**
 * I've written an API to IIT, but I'm not currently using
 *
 * TBB_USE_DEBUG
 *
 */
namespace tbb {
  namespace tbb_tools_interface {
    void task_create(void* task_address);
    void task_run(void* task_address);
    void task_complete(void* task_address);
    void task_wait_start(void* task_address);
    void task_wait_complete(void* task_address);
  }
}

#if defined(TBB_USE_PROFILING_TOOLS)
#define __TBB_TRACE_TASK_CREATE(address) tbb::tbb_tools_interface::task_create(address);
#define __TBB_TRACE_TASK_ADD_DEPENDENCY(from,to)
#define __TBB_TRACE_TASK_RUN(address) tbb::tbb_tools_interface::task_run(address);
#define __TBB_TRACE_TASK_COMPLETE(address) tbb::tbb_tools_interface::task_complete(address);
#define __TBB_TRACE_TASK_WAIT(address) tbb::tbb_tools_interface::task_wait_start(address.get());
#define __TBB_TRACE_TASK_WAIT_COMPLETE(address) tbb::tbb_tools_interface::task_wait_complete(address.get());
#define __TBB_TRACE_TASK_MEMORIZE_NODE(number,address)
#elif defined(TBB_TRACE_DYNAMIC_TASK_GRAPHS)
#include <iostream>

#define __TBB_TRACE_TASK_CREATE(address) std::cout << "TBB DEBUG: create task " << address << std::endl;
#define __TBB_TRACE_TASK_ADD_DEPENDENCY(from,to) std::cout << "TBB DEBUG: task " << from << "->" << to << std::endl;
#define __TBB_TRACE_TASK_RUN(address) std::cout << "TBB DEBUG: run task " << address << std::endl;
#define __TBB_TRACE_TASK_COMPLETE(address) std::cout << "TBB DEBUG: completed task " << address << std::endl;
#define __TBB_TRACE_TASK_WAIT(address) std::cout << "TBB DEBUG: wait for task " << address << std::endl;
#define __TBB_TRACE_TASK_WAIT_COMPLETE(address) std::cout << "TBB DEBUG: wait complete for task " << address << std::endl;
#define __TBB_TRACE_TASK_MEMORIZE_NODE(number,address) std::cout << "TBB DEBUG: memorize task description " << address << " with number " << number << std::endl;
#else
#define __TBB_TRACE_TASK_CREATE(address)
#define __TBB_TRACE_TASK_ADD_DEPENDENCY(from,to)
#define __TBB_TRACE_TASK_RUN(address)
#define __TBB_TRACE_TASK_COMPLETE(address)
#define __TBB_TRACE_TASK_WAIT(address)
#define __TBB_TRACE_TASK_WAIT_COMPLETE(address)
#define __TBB_TRACE_TASK_MEMORIZE_NODE(number,address)
#endif

