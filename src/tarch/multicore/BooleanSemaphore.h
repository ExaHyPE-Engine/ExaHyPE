// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"
#if defined(SharedCPP)
  #include "cpp/BooleanSemaphore.h"
#elif defined(SharedTBB)
  #include "tbb/BooleanSemaphore.h"
#elif defined(SharedOMP)
  #include "omp/BooleanSemaphore.h"
#elif !defined(_TARCH_MULTICORE_BOOLEAN_SEMAPHORE_H_)
#define _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_H_

#include <string>


namespace tarch {
  namespace multicore {
    class BooleanSemaphore;
    class Lock;
  }
}


/**
 * Boolean Semaphore
 *
 * A boolean semaphore is an atomic variable that protects a section/scope. If
 * k scopes are guarded by a boolean semaphore, any thread can enter this
 * scope. As soon as it enters the scope, the boolean semaphore switches and
 * all other threads that want to enter a section that is protected by the same
 * semaphore have to wait.
 *
 * ## Usage
 *
 * - Add a class attribute of BooleanSemaphore to your class.
 * - For each operation with a critical section:
 *   - Create within the operation an instance of Lock and pass it the
 *     semaphore instance.
 *   - Free the lock (optional). If you don't free it, it will be freed as soon
 *     as the  scope is left and the lock object is destroyed.
 *
 * \code
   class MyClass {
     ...

     static BooleanSemaphore _mySemaphore;
   };



   MyClass::foo() {
     ...
     Lock myLock( _mySemphore );
     // critical Section A
     ...
   }


   MyClass::bar() {
     ...
     Lock myLock( _mySemphore );
     // critical Section B
     ...
   }

 \endcode
 *
 * In the code above, the critical sections A and B are never executed at the
 * same time.
 *
 * ## Rationale
 *
 * - A pure critical section does not work for many applications: A plotter for
 *   example writes both vertices and cells to an output file. The write
 *   process has to be protected. A critical section within the vertex plotter
 *   scope does not prevent the cells to be written to the file simultaneously.
 *   This has to be forbidden by a plotter semaphore.
 * - Intel TBB's have a built-in semaphore concept called scoped lock.
 * - OpenMP has named critical sections that might work.
 * - I added this class to have a unified usage model (even without any multicore
 *   parallelisation switched on) for everybody.
 * - You cannot lock a semaphore manually as the corresponding operations are
 *   hidden. Instead, you have to create a lock object that does the lock for
 *   you and frees the semaphore as soon as the lock object is destroyed. The
 *   reason for this process is simple: This way, you cannot forget to free a
 *   semaphore (e.g. due to a return right in the middle of a critical
 *   section).
 * - The lock is a class that can be implemented without any OpenMP- or
 *   TBB-specific stuff.
 * - The specification of the semaphore is independent of the parallelisation
 *   model chosen.
 * - The subdirectories of this directory hold the TBB- and OpenMP-specific
 *   implementations of the semaphore.
 *
 *
 * \section  Bugs due to the locks
 *
 * Inserting Locks and relying on the lock destructor turns out to be
 * problematic in two cases:
 *
 * - If the Lock is used within a routine that contains a return, it seems
 *   that some compilers invoke the destructor of Lock before the return
 *   statement (regardless whether it is called or not). This means variable
 *   accesses following a branch with a return are not protected anymore.
 *
 * - If the Lock is used within a template, it seems that some compilers
 *   simply embed the code block into the calling function. The scoping
 *   rules are not preserved.
 *
 * If you run into such cases, we recommend that you call the lock's free()
 * explicitly. In our codes, this did resolve all the issues.
 *
 * @author Tobias Weinzierl
 */
class tarch::multicore::BooleanSemaphore {
  private:
    friend class tarch::multicore::Lock;
    friend class RecursiveSemaphore;

    /**
     * Waits until I can enter the critical section.
     */
    void enterCriticalSection();

    /**
     * Tells the semaphore that it is about to leave.
     */
    void leaveCriticalSection();

    /**
     * Run into critical section and try to lock. If we are successful,
     * the routine returns true and the stuff is locked (so please call
     * leave later on). Otherwise, I return false.
     */
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore( const BooleanSemaphore& semaphore ) {}

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore& operator=( const BooleanSemaphore& semaphore ) {return *this;}
  public:
    BooleanSemaphore();
    ~BooleanSemaphore();
};


#endif
