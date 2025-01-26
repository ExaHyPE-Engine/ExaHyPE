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
     * Delegate to TBB
     *
     * Basically, this is a renaming of TBB's function calls to make it
     * compatible with our manual implementation that we have to use for
     * OpenMP and native C++.
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
