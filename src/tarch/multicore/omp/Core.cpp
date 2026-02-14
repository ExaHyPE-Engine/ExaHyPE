// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Core.h"

#if defined(WITH_OPENMP)

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/multicore.h"

#ifdef COMPILER_HAS_SYSINFO
#include <sched.h>
#endif

#include <omp.h>

bool tarch::multicore::Core::init() {
  // if (omp_get_num_procs() != omp_get_max_threads()) {
  //   logDebug(
  //     "init()",
  //     "omp_get_num_procs() reports "
  //       << omp_get_num_procs() << " while omp_get_max_threads() reports " << omp_get_max_threads()
  //       << ". Take smaller value to avoid overbooking."
  //   );
  // }

  const int maxThreads = std::min(omp_get_num_procs(), omp_get_max_threads());
  omp_set_num_threads(maxThreads);
  _numberOfThreads = omp_get_max_threads();

  if (_numberOfThreads > getNumberOfUnmaskedThreads()) {
    logDebug(
      "init()",
      "Number of configured threads ("
        << _numberOfThreads << ") is bigger than available unmasked threads (" << getNumberOfUnmaskedThreads() << ")"
    );
    logDebug("init()", "Unmasked threads: " << printUnmaskedThreads());
  }

  return isInitialised();
}

void tarch::multicore::Core::shutdown() {}

bool tarch::multicore::Core::isInitialised() const { return _numberOfThreads > 0; }

int tarch::multicore::Core::getNumberOfThreads() const { return _numberOfThreads; }

int tarch::multicore::Core::getThreadNumber() const { return omp_get_thread_num(); }

int tarch::multicore::Core::getCoreNumber() const {
#ifdef COMPILER_HAS_SYSINFO
  return sched_getcpu();
#else
  //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
  return 0;
#endif
}

void tarch::multicore::Core::yield() {
#pragma omp taskyield
}

#endif
