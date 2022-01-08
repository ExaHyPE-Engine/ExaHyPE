#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


#if defined(SharedSYCL)
#include <sstream>
#include "config.h"
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>


namespace {
  oneapi::tbb::global_control*  globalControl = nullptr;
}


tarch::logging::Log tarch::multicore::Core::_log( "tarch::multicore::Core" );


tarch::multicore::Core::Core():
  _numberOfThreads( oneapi::tbb::info::default_concurrency() ),
  _numberOfDevices( -1 ) {
  auto platforms = sycl::platform::get_platforms();
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();
    _numberOfDevices = platform.get_devices().size();

    #if PeanoDebug>0
    std::cout << "found platform " << platform.get_info<sycl::info::platform::name>() << " with " << devices.size() << " device(s)" << std::endl;

    for (auto &device : devices ) {
      std::cout << "device: " << device.get_info<sycl::info::device::name>() << std::endl;
    }
    #endif
  }
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
