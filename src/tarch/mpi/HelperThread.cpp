// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HelperThread.h"

#include "tarch/Assertions.h"

tarch::mpi::HelperThread::HelperThread(std::function<bool()> pollFunction) :
  _pollFunction(std::move(pollFunction)) {}

tarch::mpi::HelperThread::~HelperThread() { stop(); }

/**
 * Starts the thread, which calls the polling function until the function returns true or stop()
 * runs. The loop never pauses between two calls, so a back-off belongs to the polling function.
 */
void
tarch::mpi::HelperThread::start() {
  assertion(not _thread.joinable());

  _stopRequested.store(false, std::memory_order_relaxed);
  _hasFinished.store(false, std::memory_order_relaxed);

  _thread = std::thread([this]() {
    while (not _stopRequested.load(std::memory_order_relaxed) and not _hasFinished.load(std::memory_order_relaxed)) {
      _hasFinished.store(_pollFunction(), std::memory_order_release);
    }
  });
}

/**
 * Stops the loop of the thread and joins the thread. A call without a started thread returns at
 * once, so the destructor calls it unconditionally.
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

/** Returns whether the polling function has returned true. */
bool
tarch::mpi::HelperThread::hasFinished() const {
  return _hasFinished.load(std::memory_order_acquire);
}
