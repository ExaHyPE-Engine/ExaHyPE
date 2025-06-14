// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/Enumerator.h"
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
 * not conflict. The present enumerator ensures exactly this, as it naturally
 * truncates the range of numbers handed out.
 */
class tarch::multicore::TaskEnumerator {
  public:
    static constexpr int NoNumber = -1;

    TaskEnumerator();
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
    static std::vector< tarch::Enumerator* >  _enumerator;
    static tarch::logging::Log                _log;
    const int                                 _enumeratorNumber;
};

