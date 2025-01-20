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


#if defined(SharedTBB) or defined(SharedTBBExtension)


#include <oneapi/tbb/spin_rw_mutex.h>

/**
 * Read/write Semaphore
 *
 * TBB-specific implementation of this class. There's a generic implementation
 * of tarch::multicore::MultiReadSingleWriteSemaphore in the parent folder.
 * However, TBB already offers exactly this feature (as opposed to other
 * multithreading runtimes), so it makes no sense not to use this bespoke
 * implementation contributed by Intel.
 */
class tarch::multicore::MultiReadSingleWriteSemaphore {
  private:
    friend class tarch::multicore::MultiReadSingleWriteLock;

    ::oneapi::tbb::spin_rw_mutex  _mutex;

    /**
     * Waits until I can enter the critical section.
     *
     * We loop until there are no write locks active anymore. The checks for
     * write locks are protected with a semaphore. Once we got our read
     * permission, we increment _numberOfActiveReadAccesses. The routine
     * leaveCriticalReadSection() will again decrement it.
     *
     * ## Fairness
     *
     * If _highPriorityToWrites is not set, we just try to get our read
     * access as quickly as possible. If this one is set however, we first
     * check if there's a write request. If so, we "deprioritise". Originally,
     * I wanted to use a while loop that waits until the write has gone
     * through:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  volatile bool letWriteAccessOvertake = _highPriorityToWrites;
  while (letWriteAccessOvertake) {
    _semaphore.enterCriticalSection(); // der create identifier, dier ja nur liest, haengt hier, d.g. write request pended
    letWriteAccessOvertake = _numberOfPendingWriteRequests>0;
    _semaphore.leaveCriticalSection();
  }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * This cannot work however, as read access can be requested recursively.
     * Let A have a read lock, B asks for write lock, and A ask for another
     * read (without releasing the first one). The snippet above produces a
     * deadlock.
     *
     * Therefore, we can grant write requests higher priority, but if and only
     * if there are no active read requests atm.
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
    /**
     * Argument is ignored in TBB version.
     */
    MultiReadSingleWriteSemaphore(bool highPriorityToWrites = true);
    ~MultiReadSingleWriteSemaphore() = default;

    using Lock = tarch::multicore::MultiReadSingleWriteLock;
};


#endif
