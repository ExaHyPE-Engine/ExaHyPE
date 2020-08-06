#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


#if defined(SharedTBB)
#include <sstream>
#include "tarch/Assertions.h"
#include "TBB.h"


// This seems to be an Intel requirement as this feature isnt' released yet officially.
#define TBB_PREVIEW_GLOBAL_CONTROL 1
#include <tbb/global_control.h>
#include <tbb/task_scheduler_init.h>


namespace {
  ::tbb::global_control*      __globalThreadCountControl;
  int                         __numberOfThreads;
}




double* tarch::multicore::allocateMemory(int size, MemoryLocation location) {
  return new double[size];
}


void tarch::multicore::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}


tarch::logging::Log  tarch::multicore::Core::_log( "tarch::multicore::Core" );


tarch::multicore::Core::Core() {
  __numberOfThreads = ::tbb::task_scheduler_init::default_num_threads();
}


tarch::multicore::Core::~Core() {
  if (__globalThreadCountControl!=nullptr) {
    logWarning( "~Core()", "please call shutdown() on core prior to shutdown" );
  }
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

void tarch::multicore::Core::configure( int numberOfThreads, int maxNumberOfConcurrentBackgroundTasks, int maxNumberOfConcurrentBandwidthBoundTasks ) {
  shutdown();

  if (numberOfThreads==UseDefaultNumberOfThreads) {
    __numberOfThreads = ::tbb::task_scheduler_init::default_num_threads();
  }
  else {
    __numberOfThreads = numberOfThreads;
    logInfo( "configure(...)", "manually reset number of threads used to " << numberOfThreads );
  }

  __globalThreadCountControl = new ::tbb::global_control(::tbb::global_control::max_allowed_parallelism,__numberOfThreads);

  if (maxNumberOfConcurrentBackgroundTasks==UseDefaultNumberOfThreads) {
    maxNumberOfConcurrentBackgroundTasks = __numberOfThreads;
  }
  if (maxNumberOfConcurrentBandwidthBoundTasks==UseDefaultNumberOfThreads ) {
    maxNumberOfConcurrentBandwidthBoundTasks = __numberOfThreads;
  }

  MaxNumberOfConcurrentBackgroundTasks     = maxNumberOfConcurrentBackgroundTasks;
  MaxNumberOfConcurrentHighBandwidthTasks  = maxNumberOfConcurrentBandwidthBoundTasks;

  if (maxNumberOfConcurrentBackgroundTasks!=UseDefaultNumberOfThreads) {
    logError( "configure(...)", "non-default number of background threads not supported" );
  }
  if (maxNumberOfConcurrentBandwidthBoundTasks!=UseDefaultNumberOfThreads) {
    logError( "configure(...)", "non-default number of high-bandwidth threads not supported" );
  }
}


void tarch::multicore::Core::shutdown() {
  logTraceIn( "shutdown()" );
  tarch::multicore::tbb::shutdownTaskEnvironment();

  if (__globalThreadCountControl!=nullptr) {
    delete __globalThreadCountControl;
  __globalThreadCountControl = nullptr;
  }

  __numberOfThreads = -1;
  logTraceOut( "shutdown()" );
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  assertion( isInitialised() );
  return __numberOfThreads;
}


std::string tarch::multicore::Core::getThreadId() const {
  std::ostringstream msg;
  msg << std::this_thread::get_id();
  return msg.str();
}


int tarch::multicore::Core::getCoreNumber() const {
  #ifdef CompilerHasSysinfo
  return sched_getcpu();
  #else
  //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
  return 1;
  #endif
}

#endif
