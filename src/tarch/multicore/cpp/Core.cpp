#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


#if defined(SharedCPP)
#include <sstream>

#include "CPP.h"


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


tarch::multicore::Core::Core():
  _numberOfThreads(std::thread::hardware_concurrency()) {
}


tarch::multicore::Core::~Core() {
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

void tarch::multicore::Core::configure( int numberOfThreads, int maxNumberOfConcurrentBackgroundTasks, int maxNumberOfConcurrentBandwidthBoundTasks ) {
  if (numberOfThreads != UseDefaultNumberOfThreads) {
    _numberOfThreads = numberOfThreads;
    if (_numberOfThreads>getNumberOfUnmaskedThreads()) {
      logWarning( "configure(int,int,int)", "number of configured threads (" << numberOfThreads << ") is bigger than available unmasked threads (" << getNumberOfUnmaskedThreads() << ")" );
    }
  }
  else {
    _numberOfThreads = std::thread::hardware_concurrency();
  }

  if (maxNumberOfConcurrentBackgroundTasks!=UseDefaultNumberOfThreads) {
    logError( "configure(...)", "non-default number of background threads not supported" );
  }
  if (maxNumberOfConcurrentBandwidthBoundTasks!=UseDefaultNumberOfThreads) {
    logError( "configure(...)", "non-default number of high-bandwidth threads not supported" );
  }
}


void tarch::multicore::Core::shutdown() {
  logTraceIn( "shutdown()" );
  tarch::multicore::cpp::shutdownConsumerTasks();
  logTraceOut( "shutdown()" );
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  return _numberOfThreads;
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
