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
 */
class tarch::multicore::MultiReadSingleWriteSemaphore {
  private:
    friend class tarch::multicore::MultiReadSingleWriteLock;

    BooleanSemaphore  _semaphore;
    bool              _writeAccessRequired;
    int               _numberOfActiveReadAccesses;

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
    MultiReadSingleWriteSemaphore( const MultiReadSingleWriteSemaphore& ) {}

    /**
     * You may not copy a semaphore
     */
    MultiReadSingleWriteSemaphore& operator = ( const MultiReadSingleWriteSemaphore& ) { return *this; }

  public:
    MultiReadSingleWriteSemaphore();
    ~MultiReadSingleWriteSemaphore();
};

