// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Enumerator.h"
#include "peano4/utils/Globals.h"

#include <map>

namespace swift2 {
  class TaskEnumerator;
}


/**
 * Task enumerator for Swift 2
 *
 * Every particle has a marker associate to the vertices and cells. Each marker
 * holds an enumerator. These enumerators may not give out the same numbers.
 * Therefore, we make the enumerator instances (of this type) all become a
 * decorator for tarch::Enumerator which delegates all calls to a global
 * instance.
 *
 * Besides the administration of the tasks, the routine also maintains a big
 * hash map of resources, which is really just a hash map to boolean semaphores
 * plus some routines that manage access to them. These can be used to ensure
 * that no two cells access the same vertex concurrently.
 */
class swift2::TaskEnumerator {
  public:
    TaskEnumerator() = default;

    void reset();

    /**
     * Create a new task number
     *
     * Returns a unique non-zero number which is not yet used anywhere else on
     * this rank. This getter also ensures that there's a resource semaphore,
     * so we can lock the underlying resource.
     */
    int getNumber();

    /**
     * Free task number
     *
     * Free a number again. number has to be the result of a previous
     * getNumber() call. While getNumber() can generate resources, we do not
     * free them in releaseNumber().
     */
    void releaseNumber( int value );

    /**
     * Number of numbers handed out so far.
     *
     * Decorator for tarch::Enumerator::size().
     */
    int size() const;

    /**
     * Decorator for tarch::Enumerator::toString().
     */
    std::string toString() const;

    /**
     * Lock resources
     *
     * This routine is used to lock the @f$ 2^d @f$ adjacent vertices of a
     * cell. This way, we ensure that noone else uses these vertices
     * concurrently.
     *
     * The implementation is not a simple while loop: We loop over the
     * entries of numbers and try to lock them. If one lock fails, we
     * unlock all previous ones immediately again, yield, and then try
     * again. This way, we avoid deadlocks if multiple cells next to each
     * other each try to lock their adjacent vertices but actually only get
     * around half of the locks they want.
     */
    static void lockResources( const int numbers[TwoPowerD] );

    /**
     * Free resources
     *
     * Counterpart to lockResources().
     */
    static void unlockResources( const int numbers[TwoPowerD] );

  private:
    static tarch::Enumerator _globalEnumerator;

    /**
     * Each task is tied to a resource, i.e. a unique number, which we can
     * lock and unlock again.
     */
    static std::map< int, tarch::multicore::BooleanSemaphore* >  _resources;
};

