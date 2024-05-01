#include "dynamic_task_graph_utils.h"

//#include <ittnotify.h>


namespace {
//  __itt_event  tbb_event_task_create = __itt_event_create("dynamic_task", trace.size() );;
}


void tbb::tbb_tools_interface::task_create(void* task_address) {}


void tbb::tbb_tools_interface::task_run(void* task_address) {}


void tbb::tbb_tools_interface::task_complete(void* task_address) {}


void tbb::tbb_tools_interface::task_wait_start(void* task_address) {}


void tbb::tbb_tools_interface::task_wait_complete(void* task_address) {}
