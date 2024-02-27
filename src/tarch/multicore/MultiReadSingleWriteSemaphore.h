// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace tarch {
  namespace multicore {
    class MultiReadSingleWriteSemaphore;
    class MultiReadSingleWriteLock;
  }
}



/**
 * Read/write Semaphore
 *
 * Semaphore where many people can get write access concurrently but only
 * one can write. If the write lock is on, no reads are allowed.
 *
 * We offer two realisation variants of the semaphore: The default variant
 * gives write accesses high priority. The alternative one gives read
 * accesses higher priority.
 *
 * The latter happens automatically if we don't do anything: If one thread
 * locks the semaphore for reads, any other thread can read as well.
 * Consequently, the probability is high that a read will pass quickly,
 * while a write basically has to wait for a slot where no reads are
 * fired anymore. You can toggle this behaviour when you create the
 * semaphore, but this has to be done with care: The semaphore has been
 * designed to allow concurrent reads, and if you give the writes the
 * higher priority, you eliminate this advantage to some degree.
 *
 * ## Realisation rationale
 *
 * I played around with sleeps, but this is a really stupid idea. It
 * makes the performance deteriorate absolutely. I then integrated the
 * two checks for number of pending write accesses and read accesses
 * into one semaphore loop, and that didn't work either. My assumption
 * is that we then tend to starve all other threads. So the best result
 * is achieved if we poll with tryEnterCriticalSection(), but then
 * check for writes and reads in two stages.
 */
class tarch::multicore::MultiReadSingleWriteSemaphore {
  private:
    friend class tarch::multicore::MultiReadSingleWriteLock;

    const bool        _highPriorityToWrites;
    BooleanSemaphore  _semaphore;
    volatile int      _numberOfPendingWriteRequests;
    volatile bool     _writeAccessAcquired;
    volatile int      _numberOfActiveReadAccesses;

    /**
     * Waits until I can enter the critical section.
     */
    void enterCriticalReadSection();
    void enterCriticalWriteSection();

    /**
     * Tells the semaphore that it is about to leave.
     */
    void leaveCriticalReadSection();
    void leaveCriticalWriteSection();

    /**
     * You may not copy a semaphore
     */
    MultiReadSingleWriteSemaphore( const MultiReadSingleWriteSemaphore& ) = delete;

    /**
     * You may not copy a semaphore
     */
    MultiReadSingleWriteSemaphore& operator = ( const MultiReadSingleWriteSemaphore& ) { return *this; }

  public:
    MultiReadSingleWriteSemaphore(bool highPriorityToWrites = true);
    ~MultiReadSingleWriteSemaphore();
};

