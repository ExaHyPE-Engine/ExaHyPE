// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HelperThread.h"

#include "tarch/Assertions.h"

tarch::mpi::HelperThread::HelperThread(std::function<bool()> pollFunction) :
  _pollFunction(std::move(pollFunction)) {}

tarch::mpi::HelperThread::~HelperThread() { stop(); }

/**
 * Launches the thread and lets it poll until the function reports completion.
 *
 * The loop itself never pauses between two passes. How expensive a pass is
 * depends on what the polling function tests, so any back-off belongs to that
 * function.
 */
void tarch::mpi::HelperThread::start() {
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
 * Asks the thread to leave its loop and waits for it.
 *
 * A thread that never started or has already been taken down is left alone, so
 * the destructor and restart() can both call this unconditionally.
 */
void tarch::mpi::HelperThread::stop() {
  if (not _thread.joinable()) {
    return;
  }

  _stopRequested.store(true, std::memory_order_relaxed);
  _thread.join();
  _thread = std::thread();
}

/** Takes the thread down and brings it back up for the next round of polling. */
void tarch::mpi::HelperThread::restart() {
  stop();
  start();
}

/** Returns whether the polling function has reported that it has nothing left to do. */
bool tarch::mpi::HelperThread::hasFinished() const { return _hasFinished.load(std::memory_order_acquire); }
