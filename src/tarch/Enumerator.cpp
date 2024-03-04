#include "Enumerator.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <limits>


tarch::Enumerator::Enumerator():
  _semaphore(),
  _activeNumbers()
{}


void tarch::Enumerator::reset() {
  tarch::multicore::Lock lock( _semaphore );
  _activeNumbers.clear();
}


int tarch::Enumerator::size() const {
  return _activeNumbers.size();
}


int tarch::Enumerator::getNumber() {
  tarch::multicore::Lock lock( _semaphore );

  int                    result = _activeNumbers.size();
  constexpr int          Increment  = 23;
  constexpr int          Max    = std::numeric_limits<int>::max() - 2 * Increment;

  assertion2(Max % Increment != 0, Increment, Max);

  while (_activeNumbers.count(result) > 0) {
    result = result % Max;
    result += Increment;
  }
  _activeNumbers.insert(result);
  return result;
}


void tarch::Enumerator::releaseNumber( int number ) {
  assertion(number >= 0);
  tarch::multicore::Lock lock( _semaphore );
  assertionEquals(_activeNumbers.count(number), 1);
  _activeNumbers.erase(number);
}


std::string tarch::Enumerator::toString() const {
  return "(size=" + std::to_string( size() ) + ")";
}
