#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


#if defined(SharedSYCL)
#include "Core.h"

#include <sstream>
#include "config.h"
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>

#include <map>


namespace {
  oneapi::tbb::global_control*  globalControl = nullptr;
  std::map< int, sycl::queue >  syclQueues;
}


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


sycl::queue& tarch::multicore::getSYCLQueue( int number ) {
  assertion1( syclQueues.count(number)==1, number );
  return syclQueues[number];
}


tarch::multicore::Core::Core():
  _numberOfThreads( oneapi::tbb::info::default_concurrency() ),
  _numberOfDevices( 0 ) {
  auto platforms     = sycl::platform::get_platforms();
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();

    for (auto &device : devices ) {
      std::cout << "device #" << _numberOfDevices << ": " << device.get_info<sycl::info::device::name>() << std::endl;
      syclQueues.insert( std::pair< int, sycl::queue >( _numberOfDevices, sycl::queue(device) ) );
      _numberOfDevices++;
    }
  }
  syclQueues.insert( std::pair< int, sycl::queue >( -1, sycl::device(sycl::cpu_selector()) ) );
  std::cout << "default device (queue -1) maps onto CPU:" << syclQueues[-1].get_device().get_info<sycl::info::device::name>() << std::endl;
}


tarch::multicore::Core::~Core() {
}


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
  }
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


int tarch::multicore::Core::getNumberOfGPUs() const {
  return _numberOfDevices;
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
