// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/Enumerator.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include <set>
#include <string>


namespace tarch {
  namespace multicore {
    class TaskEnumerator;
  }
}

/**
 * Rank-global enumerator for tasks
 *
 * The "problem" with tasks is that we often have different solvers running in
 * parallel which all need their own task numbers. And these should, obviously,
 * not conflict. The present enumerator tackles this problem:
 *
 * The class is a wrapper layer around a set of enumerators. It is aware of the
 * maximum number of tasks that our system can handle. Every time you construct
 * an instance of this class, it coordinates internally with all the other
 * instances of this class and ensures that each instance only hands out numbers
 * from its specific range.
 *
 * If you had, for example, only numbers from 0 up to 10000-1, and you create
 * two instances of tarch::multicore::TaskEnumerator, then the first instance
 * would only hand out numbers from 0 to 5000-1, and the second one only from
 * 5000 to 10000-1. They ensure that there is no conflict.
 *
 * The actual task number administration is thread-safe, as we rely internally
 * on tarch::Enumerator instances on the heap. However, the creation of these
 * heap objects can lead to race conditions. Therefore, we protect this access
 * explicitly with a semaphore allowing for multiple reads but only one write.
 */
class tarch::multicore::TaskEnumerator {
  public:
    static constexpr int NoNumber = -1;

    TaskEnumerator();
    void reset();

    /**
     * Get a new task number
     *
     * Returns a unique non-zero number which is not yet used anywhere else on
     * this rank. The function delegates the call to an instance of
     * @ref tarch::Enumerator::getNumber(). Consequently, it is
     * thread-safe.
     */
    int getNumber();

    /**
     * Release a task number
     *
     * Free a number again. number has to be the result of a previous
     * getNumber() call. Is thread-safe.
     */
    void releaseNumber( int value );

    /**
     * Number of numbers handed out so far.
     */
    int size() const;

    std::string toString() const;

  private:
    static tarch::multicore::MultiReadSingleWriteSemaphore  _enumeratorSequenceSemaphore;
    static std::vector< tarch::Enumerator* >  _enumerator;
    static tarch::logging::Log                _log;

    /**
     * Number of enumerator that's tied to this instance
     *
     * The value serves as index to access _enumerator.
     *
     * Logically, this attribute is a const attribute. We however cannot
     * make it const, as the constructor first of all has to lock the
     * array _enumerator, then it can determine what the value of
     * _enumeratorNumber number is.
     */
    int                                 _enumeratorNumber;
};

