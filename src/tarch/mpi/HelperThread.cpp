// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HelperThread.h"

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Rank.h"

#include <omp.h>

#ifdef COMPILER_HAS_SYSINFO
#include <pthread.h>
#include <sched.h>
#endif

namespace {
  /**
   * Restricts thread to the logical processors of the OpenMP places that no thread of the team
   * occupies. Inside a parallel region, without places, or with every place occupied, thread keeps
   * the affinity that it inherits from the calling thread. The last case and a failed binding log a
   * warning.
   */
  void bindToUnoccupiedPlaces([[maybe_unused]] std::thread& thread) {
#ifdef COMPILER_HAS_SYSINFO
    const int numberOfPlaces = omp_get_num_places();
    if (numberOfPlaces <= 0 or omp_in_parallel() != 0) {
      return;
    }
    std::vector<int> occupiedPlaces(static_cast<std::size_t>(numberOfPlaces), 0);
#pragma omp parallel default(none) shared(occupiedPlaces)
    {
      const int place = omp_get_place_num();
      if (place >= 0) {
#pragma omp atomic write
        occupiedPlaces[static_cast<std::size_t>(place)] = 1;
      }
    }
    cpu_set_t processors;
    CPU_ZERO(&processors);
    int numberOfProcessors = 0;
    for (int place = 0; place < numberOfPlaces; place++) {
      if (occupiedPlaces[static_cast<std::size_t>(place)] != 0) {
        continue;
      }
      std::vector<int> processorsOfPlace(static_cast<std::size_t>(omp_get_place_num_procs(place)));
      omp_get_place_proc_ids(place, processorsOfPlace.data());
      for (const int processor : processorsOfPlace) {
        CPU_SET(processor, &processors);
        numberOfProcessors++;
      }
    }
    if (numberOfProcessors > 0) {
      if (pthread_setaffinity_np(thread.native_handle(), sizeof(processors), &processors) != 0) {
        logWarning("The communication thread cannot bind to the CPUs that no OpenMP thread uses.");
      }
    } else {
      logWarning(
        "The communication thread needs a CPU that no OpenMP thread uses. Set OMP_NUM_THREADS below "
        "the number of CPUs of a rank."
      );
    }
#endif
  }

  /**
   * Ends the rank when exit() runs on the thread of this object. A call of exit() destroys the
   * thread-local objects of its thread before every static object. The other threads of the rank
   * still read the static objects at that time.
   */
  class RankEndAtExit final {
  public:
    RankEndAtExit()                                = default;
    RankEndAtExit(const RankEndAtExit&)            = delete;
    RankEndAtExit(RankEndAtExit&&)                 = delete;
    RankEndAtExit& operator=(const RankEndAtExit&) = delete;
    RankEndAtExit& operator=(RankEndAtExit&&)      = delete;
    ~RankEndAtExit() {
      if (not _loopHasEnded) {
        tarch::mpi::endRankWithErrorCodeOfAbort();
      }
    }

    void setLoopHasEnded() { _loopHasEnded = true; }

  private:
    bool _loopHasEnded{false};
  };
} // namespace

tarch::mpi::HelperThread::HelperThread(std::function<bool()> pollFunction) :
  _pollFunction(std::move(pollFunction)) {}

tarch::mpi::HelperThread::~HelperThread() { stop(); }

/**
 * Starts the thread on the logical processors that the OpenMP team leaves free. The thread calls
 * the polling function until the function returns true or stop() runs. A back-off belongs to the
 * polling function.
 */
void
tarch::mpi::HelperThread::start() {
  assertion(not _thread.joinable());

  _stopRequested.store(false, std::memory_order_relaxed);
  _hasFinished.store(false, std::memory_order_relaxed);

  _thread = std::thread([this]() {
    thread_local RankEndAtExit rankEndAtExit;
    while (not _stopRequested.load(std::memory_order_relaxed) and not _hasFinished.load(std::memory_order_relaxed)) {
      _hasFinished.store(_pollFunction(), std::memory_order_release);
    }
    rankEndAtExit.setLoopHasEnded();
  });
  bindToUnoccupiedPlaces(_thread);
}

/**
 * Stops the loop of the thread and joins the thread. A call without a started thread returns at
 * once. The destructor calls it unconditionally.
 */
void
tarch::mpi::HelperThread::stop() {
  if (not _thread.joinable()) {
    return;
  }

  _stopRequested.store(true, std::memory_order_relaxed);
  _thread.join();
  _thread = std::thread();
}

/**
 * Returns whether the polling function has returned true.
 */
bool
tarch::mpi::HelperThread::hasFinished() const {
  return _hasFinished.load(std::memory_order_acquire);
}
