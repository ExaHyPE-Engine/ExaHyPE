// Copyright (C) 2009 Technische Universitaet Muenchen
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MPI_LOCK_H_
#define _TARCH_MPI_LOCK_H_


namespace tarch {
  namespace mpi {
    class BooleanSemaphore;
    class Lock;
  }
}


/**
 * Create a lock around a boolean semaphore region
 *
 * @see tarch::multicore::BooleanSemaphore
 */
class tarch::mpi::Lock {
  private:
    BooleanSemaphore&  _semaphore;
    bool               _lockIsAquired;
  public:
    Lock( tarch::mpi::BooleanSemaphore& semaphore, bool aquireLockImmediately = true );
    ~Lock();

    void lock();
    void free();
};

#endif
