// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
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
 *
 * The object is thread-safe, i.e. all modifiers are protected via a semaphore.
 *
 * It ensures that no number is handed out twice, by maintaining a set of
 * _activeNumbers.
 */
class tarch::Enumerator {
  public:
    static constexpr int NoNumber = -1;

    Enumerator(int max = std::numeric_limits<int>::max()-2 );

    /**
     * Complete reset, which basically eliminates all the active numbers.
     */
    void reset();

    void setMax( int max );

    /**
     * Return unique number
     *
     * Returns a unique non-zero number which is not yet used anywhere else on
     * this rank. The number is positive or zero.
     */
    int getNumber();

    /**
     * Free a number.
     *
     * number has to be the result of a previous getNumber() call. We had the
     * situation where enclave task numbers became really big as we just
     * increment _lastNumberHandedOut in getNumber(). So what I do is that I
     * reduce _lastNumberHandedOut by half if value is too big. This way, we
     * at least scan only half of the overall number range that we might have
     * at our disposal.
     *
     * @param value The number that we want to release. If you pass in
     *   NoNumber, then nothing happens.
     */
    void releaseNumber( int value );

    /**
     * Number of numbers handed out so far.
     */
    int size() const;

    std::string toString() const;

  private:
    static tarch::logging::Log         _log;

    tarch::multicore::BooleanSemaphore _semaphore;
    std::set<int>                      _activeNumbers;
    int                                _max;
    int                                _lastNumberHandedOut;
};

