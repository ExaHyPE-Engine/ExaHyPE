#include "tarch/Assertions.h"
#include "Core.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

#include <map>




#if defined(SharedSYCL)
#include <sstream>
#include "config.h"


#include <map>


#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


namespace {
  //sycl::queue  hostSyclQueue( sycl::device(sycl::cpu_selector()) );
  //sycl::queue  hostSyclQueue( sycl::cpu_selector_v() );
  sycl::queue  hostSyclQueue;
}


sycl::queue& tarch::multicore::getHostSYCLQueue() {
  return hostSyclQueue;
}


/**
 * The SYCL core object differs from other core objects in that is solely makes
 * the host available for task "offloading".
 */
tarch::multicore::Core::Core():
  _numberOfThreads( 0 ) {

  #if PeanoDebug>0
  std::cout << "host (CPU) device queue:" << hostSyclQueue.get_device().get_info<sycl::info::device::name>() << std::endl;
  #endif

  internal::configureInternalTaskQueues(_numberOfThreads);
  _numberOfThreads = hostSyclQueue.get_device().get_info<sycl::info::device::max_compute_units>();
}


tarch::multicore::Core::~Core() {
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}


void tarch::multicore::Core::configure( int numberOfThreads ) {
  if ( numberOfThreads!=_numberOfThreads) {
    logWarning( "configure(int)", "can not reconfigure number of threads within SYCL queue. Will work with " << numberOfThreads << " logical threads within host queue having " << _numberOfThreads << " threads" );
    _numberOfThreads = numberOfThreads;
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
