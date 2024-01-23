// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"
#include <set>
#include <string>


namespace tarch {
  class Enumerator;
}

/**
 * Simple rank-global enumerator
 *
 * Very simple utility class to build up a global set of numbers on a rank. Is
 * used in various places to create task numbers for example or to enumerate
 * all vertices and cells properly.
 */
class tarch::Enumerator {
  public:
    static constexpr int NoNumber = -1;

    Enumerator();
    void reset();

    /**
     * Returns a unique non-zero number which is not yet used anywhere else on
     * this rank.
     */
    int getNumber();

    /**
     * Free a number again. number has to be the result of a previous
     * getNumber() call.
     */
    void releaseNumber( int value );

    /**
     * Number of numbers handed out so far.
     */
    int size() const;

    std::string toString() const;

  private:
    tarch::multicore::BooleanSemaphore _semaphore;
    std::set<int>                      _activeNumbers;
};

