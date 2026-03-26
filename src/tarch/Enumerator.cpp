#include "Enumerator.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <limits>


tarch::logging::Log         tarch::Enumerator::_log( "tarch::Enumerator" );


tarch::Enumerator::Enumerator(int max):
  _semaphore(),
  _activeNumbers(),
  _max(max),
  _lastNumberHandedOut(-1)
{}


void tarch::Enumerator::reset() {
  tarch::multicore::Lock lock( _semaphore );
  _activeNumbers.clear();
  _lastNumberHandedOut = -1;
}


int tarch::Enumerator::size() const {
  return _activeNumbers.size();
}


int tarch::Enumerator::getNumber() {
  tarch::multicore::Lock lock( _semaphore );

  int result = NoNumber;

  if ( size()<_max ) {
    _lastNumberHandedOut++;

    while (_activeNumbers.count(_lastNumberHandedOut) > 0) {
      _lastNumberHandedOut++;
      _lastNumberHandedOut = _lastNumberHandedOut % _max;
    }

    _activeNumbers.insert(_lastNumberHandedOut);
    result = _lastNumberHandedOut;
  }

  return result;
}


void tarch::Enumerator::setMax( int max ) {
  for (auto& p: _activeNumbers) {
    if (p>=_max) {
      logError( "setMax(max)", "had handed out " << p << " while new max is set to " << max );
      assertion(false);
    }
  }
  _max = max;
}


void tarch::Enumerator::releaseNumber( int number ) {
  assertion1(number >= 0, number);
  tarch::multicore::Lock lock( _semaphore );

  assertion2(number==NoNumber or _activeNumbers.count(number)==1, number, _activeNumbers.count(number));

  if (number==NoNumber) {
    assertionEquals(_activeNumbers.count(number), 0);
  }
  else {
    assertionEquals(_activeNumbers.count(number), 1);
    _activeNumbers.erase(number);
    if (number>=_lastNumberHandedOut) {
      _lastNumberHandedOut /= 2;
    }
  }
}


std::string tarch::Enumerator::toString() const {
  return "(size=" + std::to_string( size() ) + ")";
}
