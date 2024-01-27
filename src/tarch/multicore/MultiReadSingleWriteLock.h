// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace tarch {
  namespace multicore {
    class MultiReadSingleWriteSemaphore;
    class MultiReadSingleWriteLock;
  }
}


/**
 * Create a lock around a boolean semaphore region
 *
 * @see tarch::multicore::BooleanSemaphore
 */
class tarch::multicore::MultiReadSingleWriteLock {
  private:
    MultiReadSingleWriteSemaphore&  _semaphore;
    const bool                      _isReadLock;
    bool                            _lockIsAquired;
  public:
    static constexpr bool Read  = true;
    static constexpr bool Write = false;

    /**
     * Construct lock
     *
     * We have to know what semaphore to use and if it is a read or a write
     * lock.
     */
    MultiReadSingleWriteLock( tarch::multicore::MultiReadSingleWriteSemaphore& semaphore, bool isReadLock, bool aquireLockImmediately = true );
    ~MultiReadSingleWriteLock();

    void lock();
    void free();
};

