// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  namespace multicore {
    class RecursiveSemaphore;
    class RecursiveLock;
  }
}

/**
 * Create a lock around a boolean semaphore region
 *
 * @see tarch::multicore::BooleanSemaphore
 */
class tarch::multicore::RecursiveLock {
  private:
    RecursiveSemaphore&  _semaphore;
    bool                 _lockIsAquired;

  public:
    RecursiveLock( tarch::multicore::RecursiveSemaphore& semaphore, bool aquireLockImmediately = true );
    ~RecursiveLock();

    bool tryLock();
    bool isLocked() const;
    void lock();
    void free();
};
