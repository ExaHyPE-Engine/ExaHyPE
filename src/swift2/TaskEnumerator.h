// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Enumerator.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"

#include <map>

namespace swift2 {
  class TaskEnumerator;
}


/**
 * Task enumerator for Swift 2
 *
 * Every particle type (species) has a marker associate to the vertices and
 * cells. So they are all unique numbers. Therefore, each of these markers
 * is tied to one rank-global enumerator. These enumerators may not give out
 * the same numbers even for different species.
 * Therefore, we make the enumerator instances (of this type) all become a
 * decorator for tarch::Enumerator which delegates all calls to a global
 * instance. As a result, each marker type can work with its own instance of
 * this TaskEnumerator, but behind the scenes, all requests are funneled
 * through to one big number generator.
 *
 * Besides the administration of the tasks, the type also maintains a big
 * hash map of resources, which is really just a hash map to boolean semaphores
 * plus some routines that manage access to them. These can be used to ensure
 * that no two cells access the same vertex concurrently.
 *
 * We can use objects of this type as proxy to access a big global
 * table with one lock per global single task number. So if we have a cell
 * with four neighbours where race conditions might occur, we can use
 * lockResources() to ensure that we exclusively access those neighbours.
 * In Swift's original terminology, these are the conflicts that we resolve
 * at runtime.
 *
 *
 * ## Thread safety
 *
 * The class is thread-safe as the underlying tarch::Enumerator object is
 * thread-safe. However, we need an additional semaphore for the underlying
 * map of resources.
 *
 *
 */
class swift2::TaskEnumerator {
  public:
    TaskEnumerator() = default;

    void reset();

    /**
     * Create a new task number
     *
     * Returns a unique non-zero number which is not yet used anywhere else on
     * this rank.
     *
     *
     * ## Correlation to resources
     *
     * Resources are tied to numbers in the way that each vertex and cell
     * number plus its underlying even (touch vertex first, touch cell, touch
     * vertex last) identifies one unique counter: In practice, each number
     * that we create can yield up to three resource entries. We don't know
     * the correlation at this point, so we cannot lock any resources at this
     * point.
     *
     *
     * ## Thread safety
     *
     * The tarch::Enumerator is thread safe, so we don't have to protect the
     * actual result number generation. However, once we have a new number,
     * we also check if the resource map is properly initialised. For this, we
     * need a semaphore, as we might alter the container.
     */
    int getNumber();

    /**
     * Free task number
     *
     * Free a number again. number has to be the result of a previous
     * getNumber() call. While getNumber() can generate resources, we do not
     * free them in releaseNumber().
     *
     * @see getNumber()
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
     * This routine is used to lock a set of indices. This could be
     * the @f$ 2^d @f$ adjacent vertices of a
     * cell. This way, we ensure that noone else uses these vertices
     * concurrently. It can also be used, obviously, to lock the @f$ 2^d @f$
     * adjacent cells of a vertex in case we know their indices.
     *
     * The implementation is not a simple while loop: We loop over the
     * entries of numbers and try to lock them. If one lock fails, we
     * unlock all previous ones immediately again, yield, and then try
     * again. This way, we avoid deadlocks if multiple cells next to each
     * other each try to lock their adjacent vertices but actually only get
     * around half of the locks they want.
     *
     * Entries set to tarch::Enumerator::NoNumber are ignored.
     *
     *
     * ## Thread-safety
     *
     * At any point throughout the calculation, another thread might
     * insert new resource identifiers into the _resources map
     * through getNumber() calls. If such a call happens at exactly the
     * same time as _resources.at( numbers[counter] ), we might end up
     * with a seg fault. Therefore, the locking itself is protected
     * until we get all resources.
     *
     *
     * ## Lazy generation of indices
     *
     * getNumber() explains why there's no direct 1:1 match of numbers to
     * resources. Therefore, we create indices lazily at this point through
     * insertNumbers().
     */
    static void lockResources( const tarch::la::Vector<TwoPowerD,int>& numbers );

    /**
     * Free resources
     *
     * Counterpart to lockResources().
     *
     * Entries set to tarch::Enumerator::NoNumber are ignored.
     */
    static void unlockResources( const tarch::la::Vector<TwoPowerD,int>& numbers );

  private:
    static tarch::Enumerator _globalEnumerator;

    /**
     * Each task is tied to a resource, i.e. a unique number, which we can
     * lock and unlock again.
     */
    static std::map< int, tarch::multicore::BooleanSemaphore* >  _resources;

    /**
     * Protect the _resources map
     *
     * This map holds all the boolean semaphores. An arbitrary number of
     * threads can hit this map at the same time and try to book some
     * semaphores. This is technically a read access, as they do not
     * write to the map. They only read from the map.
     *
     * Contrary to this, we may not change the map, i.e. insert new
     * semaphores, if others try to read it, as new inserts might change
     * the underlying memory arrangement and hence mess everything up.
     */
    using ResourcesSemaphore = tarch::multicore::MultiReadSingleWriteSemaphore;

    static ResourcesSemaphore   _resourcesSemaphore;

    /**
     * Create a new number
     *
     * This routine is robust, i.e. if the number does exist already, we do not
     * do anything.
     */
    static void insertNumber(int number);
    static void insertNumbers(const tarch::la::Vector<TwoPowerD,int>& numbers);
};

