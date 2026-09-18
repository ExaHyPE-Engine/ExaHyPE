// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Runtime.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Rank.h"

#include <omp.h>

#ifdef COMPILER_HAS_SYSINFO
#include <sched.h>
#endif

#ifdef __GLIBC__
#include <malloc.h>
#endif

/**
 * Prints the affinity mask of the calling process.
 *
 * An `x` denotes an available logical processor and an `o` a masked one.
 */
std::string tarch::tasking::printUnmaskedThreads() {
  std::ostringstream output;
  output << "(";
#ifdef COMPILER_HAS_SYSINFO
  cpu_set_t mask{};
  if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0) {
    const auto numberOfProcessors =
      std::min(static_cast<unsigned int>(CPU_SETSIZE), std::thread::hardware_concurrency());
    for (auto processorIndex = 0u; processorIndex < numberOfProcessors; processorIndex++) {
      if (CPU_ISSET(processorIndex, &mask) != 0) {
        output << "x";
      } else {
        output << "o";
      }
    }
  } else {
    output << " -- not available -- ";
  }
#else
  output << " -- not available -- ";
#endif
  output << ")";
  return output.str();
}

/**
 * Counts the logical processors available through the process affinity mask.
 */
int tarch::tasking::getNumberOfUnmaskedThreads() {
#ifdef COMPILER_HAS_SYSINFO
  cpu_set_t mask{};
  if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) != 0) {
    return std::max(1, omp_get_num_procs());
  }

  int result = 0;
  for (auto processorIndex = 0; processorIndex < CPU_SETSIZE; processorIndex++) {
    if (CPU_ISSET(processorIndex, &mask) != 0) {
      result++;
    }
  }

  return result;
#else
  return static_cast<int>(std::thread::hardware_concurrency());
#endif
}

tarch::tasking::Runtime& tarch::tasking::Runtime::getInstance() {
  static tarch::tasking::Runtime instance{};
  return instance;
}

bool tarch::tasking::Runtime::init() {
#ifdef __GLIBC__
  // Glibc grows each thread's arena through mprotect, and the kernel
  // serialises those calls across the threads of the first parallel sweep. A
  // large top padding takes the arena growth out of that sweep. A sanitizer or
  // a replaced allocator answers every mallopt() option with zero, and its
  // arena has no top padding to set.
  if (mallopt(M_TOP_PAD, 128 * 1024 * 1024) == 0) {
    logDebug("The allocator rejected the requested top padding.");
  }
#endif
  // A team thread that spins at a barrier takes the core that the master of
  // another rank on the same node needs, so the teams of such a node sleep
  // while they wait. The OpenMP runtime reads the variable once when it
  // starts, so it has to be set before the first OpenMP call, and a value the
  // user set wins.
  const auto& rank = tarch::mpi::Rank::getInstance();
  if (rank.getNumberOfRanks() > rank.getNumberOfNodes() and std::getenv("OMP_WAIT_POLICY") == nullptr) {
    setenv("OMP_WAIT_POLICY", "passive", 0);
#ifdef KMP_VERSION_MAJOR
    // The LLVM runtime also takes the policy after it has started, so these
    // two calls set it for a runtime that a log line started before this call.
    kmp_set_library_throughput();
    kmp_set_blocktime(0);
#endif
  }
  int maximumNumberOfThreads = std::min(omp_get_num_procs(), omp_get_max_threads());
  // The dedicated communication thread polls continuously and would otherwise
  // sit on the core of a team thread, which slows both down.
  if (tarch::mpi::AbstractCommunicationManager::usesDedicatedThread() and maximumNumberOfThreads > 1) {
    maximumNumberOfThreads--;
  }
  omp_set_num_threads(maximumNumberOfThreads);
  _numberOfThreads = omp_get_max_threads();

  if (_numberOfThreads > tarch::tasking::getNumberOfUnmaskedThreads()) {
    logDebug(
      "The number of configured threads (" << _numberOfThreads
                                           << ") is greater than the number of available unmasked threads ("
                                           << tarch::tasking::getNumberOfUnmaskedThreads() << ")."
    );
    logDebug("Unmasked threads: " << tarch::tasking::printUnmaskedThreads() << ".");
  }

  return tarch::tasking::Runtime::isInitialised();
}

void tarch::tasking::Runtime::shutdown() {}

bool tarch::tasking::Runtime::isInitialised() const { return _numberOfThreads > 0; }

int tarch::tasking::Runtime::getNumberOfThreads() const { return _numberOfThreads; }

int tarch::tasking::Runtime::getThreadNumber() const { return omp_get_thread_num(); }

int tarch::tasking::Runtime::getCoreNumber() const {
#ifdef COMPILER_HAS_SYSINFO
  return sched_getcpu();
#else
  return 0;
#endif
}
