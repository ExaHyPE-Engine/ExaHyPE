// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Core.h"

#if defined(SharedSYCL)

#include "tarch/Assertions.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/multicore.h"
#include "tarch/CompilerSpecificSettings.h"

#include <map>

#ifdef COMPILER_HAS_SYSINFO
#include <sched.h>
#endif

#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")

namespace {
  sycl::queue hostSyclQueue(sycl::cpu_selector_v);
}

void* tarch::multicore::getHostSYCLQueue() {
  return &hostSyclQueue;
}

bool tarch::multicore::Core::init() {
  sycl::device cpuDevice;
  try {
    cpuDevice = sycl::device(sycl::cpu_selector_v);
  } catch (...) {
    logError("init()", "Failed at selecting CPU device");
    return false;
  }

  sycl::queue cpuQueue(cpuDevice);
  hostSyclQueue = cpuQueue;
  _numberOfThreads = hostSyclQueue.get_device().get_info<sycl::info::device::max_compute_units>();

  logInfo(
    "init()",
    "Host (CPU) device queue: " << hostSyclQueue.get_device().get_info<sycl::info::device::name>() << " with " << _numberOfThreads << " threads"
  );

  return isInitialised();
}

void tarch::multicore::Core::shutdown() {}

bool tarch::multicore::Core::isInitialised() const {
  return _numberOfThreads > 0;
}

int tarch::multicore::Core::getNumberOfThreads() const {
  return _numberOfThreads;
}

int tarch::multicore::Core::getCoreNumber() const {
#ifdef COMPILER_HAS_SYSINFO
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
  hostSyclQueue.wait();
}

#endif // defined(SharedSYCL)
