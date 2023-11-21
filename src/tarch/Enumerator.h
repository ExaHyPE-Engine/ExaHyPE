// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"
#include <set>


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
	int getNumber();
	void releaseNumber( int value );
	int size();

  private:
    tarch::multicore::BooleanSemaphore _semaphore;
    std::set<int>                      _activeNumbers;
};

