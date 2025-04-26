// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Core.h"

#if defined(SharedSYCL)

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/multicore.h"
#include "tarch/CompilerSpecificSettings.h"

#include <map>

#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")

tarch::logging::Log tarch::multicore::Core::_log("tarch::multicore::Core");

namespace {
  sycl::queue hostSyclQueue(sycl::cpu_selector_v);
}

void* tarch::multicore::getHostSYCLQueue() {
  return &hostSyclQueue;
}

tarch::multicore::Core::Core():
  _numberOfThreads(-1) {

  sycl::device cpuDevice;
  try {
    cpuDevice = sycl::device(sycl::cpu_selector_v);
  } catch (...) {
    std::cerr << "Warning, failed at selecting cpu device" << std::endl;
    std::abort();
  }

  sycl::queue cpuQueue( cpuDevice );
  hostSyclQueue = cpuQueue;

  _numberOfThreads = hostSyclQueue.get_device().get_info<sycl::info::device::max_compute_units>();
  assertion(_numberOfThreads>0);

  std::cout << "host (CPU) device queue: " << hostSyclQueue.get_device().get_info<sycl::info::device::name>()
            << " with " << _numberOfThreads << " threads" << std::endl;
}

tarch::multicore::Core::~Core() {}

tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

void tarch::multicore::Core::configure(int numberOfThreads) {
  if ( numberOfThreads!=tarch::multicore::Core::UseDefaultNumberOfThreads ) {
    if ( numberOfThreads!=_numberOfThreads ) {
      logWarning( "configure(int)", "cannot reconfigure number of threads within SYCL queue. Will work with " << numberOfThreads << " logical threads within host queue having " << _numberOfThreads << " threads" );
    }
    _numberOfThreads = numberOfThreads;
  }
}

void tarch::multicore::Core::shutdown() {}

bool tarch::multicore::Core::isInitialised() const {
  return true;
}

int tarch::multicore::Core::getNumberOfThreads() const {
  assertion(_numberOfThreads>0);
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
  return 0;
}

void tarch::multicore::Core::yield() {
  // @todo It would be nice to have the opportunity to tell the queue to do
  //       only a few tasks and then to return.
  hostSyclQueue.wait();
}

#endif // defined(SharedSYCL)
