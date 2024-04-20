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
    const bool         _freeLockInDestructor;
  public:
    /**
     * Create lock around semaphore
     *
     * @param aquireLockImmediately If set, the constructor locks the semaphore
     *   immediately when you construct the object. If the parameter is
     *   overwritten with false, you have to use the lock's lock()
     *   operation.
     *
     * @param freeLockInDestructor If set and if the lock has locked the
     *   semaphore, it will free it in the destructor unless you have manually
     *   unlocked it before. This parameter should be altered very carefully.
     *
     */
    Lock(
      tarch::multicore::BooleanSemaphore& semaphore,
      bool aquireLockImmediately = true,
      bool freeLockInDestructor = true
    );
    ~Lock();

    void lock();
    void free();
    bool tryLock();
};

