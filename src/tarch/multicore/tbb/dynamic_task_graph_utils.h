#pragma once


#if defined(TBB_USE_DEBUG)
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

