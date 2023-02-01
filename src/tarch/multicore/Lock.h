// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  namespace multicore {
    class BooleanSemaphore;
    class Lock;
  }
}

/**
 * Create a lock around a boolean semaphore region
 *
 * @see tarch::multicore::BooleanSemaphore
 */
class tarch::multicore::Lock {
  private:
    BooleanSemaphore&  _semaphore;
    bool               _lockIsAquired;

  public:
    Lock( tarch::multicore::BooleanSemaphore& semaphore, bool aquireLockImmediately = true );
    ~Lock();

    void lock();
    void free();
};

