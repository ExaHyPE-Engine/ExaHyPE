// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Runtime.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Rank.h"

#include <omp.h>

#ifdef COMPILER_HAS_SYSINFO
#include <sched.h>
#endif

#ifdef __GLIBC__
#include <malloc.h>
#endif

/**
 * Returns the affinity mask of the calling thread with an x for every available logical processor
 * and an o for every masked one.
 */
std::string
tarch::tasking::printUnmaskedThreads() {
  std::ostringstream output;
  output << "(";
#ifdef COMPILER_HAS_SYSINFO
  cpu_set_t mask{};
  if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0) {
    const auto numberOfProcessors =
      std::min(static_cast<unsigned int>(CPU_SETSIZE), std::thread::hardware_concurrency());
    for (auto processorIndex = 0U; processorIndex < numberOfProcessors; processorIndex++) {
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
 * Returns the number of logical processors in the affinity mask of the calling thread.
 */
int
tarch::tasking::getNumberOfUnmaskedThreads() {
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

/**
 * Returns whether the ranks on this node start more threads than the node or the affinity mask of
 * this rank has logical processors. The thread count comes from OMP_NUM_THREADS. The OpenMP runtime
 * fixes its wait policy when it starts.
 */
bool
tarch::tasking::oversubscribesNode() {
  const int unmaskedThreads = getNumberOfUnmaskedThreads();
  int       threadsPerRank  = unmaskedThreads;
  if (const char* const variable = std::getenv("OMP_NUM_THREADS"); variable != nullptr) {
    const std::string_view value(variable);
    int                    requestedThreads = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), requestedThreads);
    if (error == std::errc() and requestedThreads > 0) {
      threadsPerRank = requestedThreads;
    }
  }
  const int processorsOfNode = std::max(static_cast<int>(std::thread::hardware_concurrency()), unmaskedThreads);
  return threadsPerRank > unmaskedThreads
    or tarch::mpi::Rank::getInstance().getNumberOfRanksOnNode() * threadsPerRank > processorsOfNode;
}

tarch::tasking::Runtime&
tarch::tasking::Runtime::getInstance() {
  static tarch::tasking::Runtime instance{};
  return instance;
}

bool
tarch::tasking::Runtime::init() {
#ifdef __GLIBC__
  // A large top padding moves the arena growth of glibc and its serialised mprotect calls out of the
  // first parallel sweep. A sanitizer or a replaced allocator rejects every mallopt() option.
  if (mallopt(M_TOP_PAD, 128 * 1024 * 1024) == 0) {
    logDebug("The allocator rejected the requested top padding.");
  }
#endif
  // On an oversubscribed node a team thread that spins at a barrier takes the core of another rank.
  // The teams of such a node sleep while they wait, unless the user has set the variable.
  if (oversubscribesNode() and std::getenv("OMP_WAIT_POLICY") == nullptr) {
    setenv("OMP_WAIT_POLICY", "passive", 0);
#ifdef KMP_VERSION_MAJOR
    // The LLVM runtime also takes the policy after it has started. These two calls set it for a
    // runtime that a log line started before this call.
    kmp_set_library_throughput();
    kmp_set_blocktime(0);
#endif
  }
  const int maximumNumberOfThreads = std::min(omp_get_num_procs(), omp_get_max_threads());
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

void
tarch::tasking::Runtime::shutdown() {}

bool
tarch::tasking::Runtime::isInitialised() const {
  return _numberOfThreads > 0;
}

int
tarch::tasking::Runtime::getNumberOfThreads() const {
  return _numberOfThreads;
}

int
tarch::tasking::Runtime::getThreadNumber() const {
  return omp_get_thread_num();
}

int
tarch::tasking::Runtime::getCoreNumber() const {
#ifdef COMPILER_HAS_SYSINFO
  return sched_getcpu();
#else
  return 0;
#endif
}
