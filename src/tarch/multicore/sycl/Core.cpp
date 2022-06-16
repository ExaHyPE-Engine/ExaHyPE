#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include "tarch/multicore/sycl/SYCL.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

#include <map>




#if defined(SharedSYCL)
#include <sstream>
#include "config.h"


#include <map>


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


tarch::multicore::Core::Core():
  _numberOfThreads( 0 ),
  _numberOfDevices( 0 ) {
  internal::configureInternalTaskQueues(_numberOfThreads);
  initSYCLQueues();
  _numberOfDevices = getNumberOfSYCLQueues();
  _numberOfThreads = getSYCLQueue(HostDefaultQueue).get_device().get_info<sycl::info::device::max_compute_units>();
}


tarch::multicore::Core::~Core() {
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}


void tarch::multicore::Core::configure( int numberOfThreads ) {
  if ( numberOfThreads!=_numberOfThreads) {
    logWarning( "configure(int)", "can not reconfigure number of threads with SYCL atm" );
  }
}


void tarch::multicore::Core::shutdown() {
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  return _numberOfThreads;
}


int tarch::multicore::Core::getNumberOfGPUs() const {
  return _numberOfDevices;
}


int tarch::multicore::Core::getCoreNumber() const {
  #ifdef CompilerHasSysinfo
  return sched_getcpu();
  #else
  //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
  return -1;
  #endif
}


int tarch::multicore::Core::getThreadNumber() const {
  return -1;
}


#endif
