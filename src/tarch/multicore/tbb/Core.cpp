#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

#include <map>

#if defined(SharedTBB)
#include <sstream>
#include "config.h"
#include <tbb/global_control.h>
#include <tbb/info.h>
#include <oneapi/tbb/task_arena.h>
//#include <tbb/task_arena.h>

#include <map>

namespace {
  ::tbb::global_control* globalControl = nullptr;
}


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


tarch::multicore::Core::Core():
  _numberOfThreads( ::tbb::info::default_concurrency() ) {
  internal::configureInternalTaskQueues(_numberOfThreads);
}


tarch::multicore::Core::~Core() {}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}


void tarch::multicore::Core::configure( int numberOfThreads ) {
  if (globalControl!=nullptr) delete globalControl;
  globalControl = nullptr;
  if (numberOfThreads != UseDefaultNumberOfThreads) {
    _numberOfThreads = numberOfThreads;
    globalControl    = new oneapi::tbb::global_control(oneapi::tbb::global_control::max_allowed_parallelism,numberOfThreads);
  }
  else {
    _numberOfThreads = oneapi::tbb::info::default_concurrency();
  }

  if (_numberOfThreads>getNumberOfUnmaskedThreads()) {
    logWarning( "configure(int)", "number of configured threads (" << numberOfThreads << ") is bigger than available unmasked threads (" << getNumberOfUnmaskedThreads() << ")" );
    logWarning( "configure(int)", "unmasked threads: " << printUnmaskedThreads() );
  }

  internal::configureInternalTaskQueues(_numberOfThreads);
}


void tarch::multicore::Core::shutdown() {
  if (globalControl!=nullptr) delete globalControl;
  globalControl = nullptr;
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  return _numberOfThreads;
}


int tarch::multicore::Core::getCoreNumber() const {
  #ifdef CompilerHasSysinfo
    return sched_getcpu();
  #else
    //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
    return 1;
  #endif
}


int tarch::multicore::Core::getThreadNumber() const {
  return tbb::this_task_arena::current_thread_index();
}


void tarch::multicore::Core::yield() {
  // @todo This is not what I want. I want to interrupt the current task and
  //       tell TBB to continue with another task. This is technically not a
  //       yield, as I do not expect another thread to come in
  std::this_thread::yield();
}
#endif
