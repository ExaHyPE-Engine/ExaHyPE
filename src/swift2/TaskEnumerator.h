// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Enumerator.h"

namespace swift2 {
  class TaskEnumerator;
}


/**
 * Task enumerator for Swift 2
 *
 * Every particle has a marker associate to the vertices and cells. Each marker
 * holds an enumerator. These enumerator may not give out the same numbers.
 * So this class is basically a decorator of tarch::Enumerator which delegates
 * all calls to a global instance.
 */
class swift2::TaskEnumerator {
  public:
    TaskEnumerator() = default;

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
    static tarch::Enumerator _globalEnumerator;

};

