#include "TaskEnumerator.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <limits>


std::vector< tarch::Enumerator* >   tarch::multicore::TaskEnumerator::_enumerator;
tarch::logging::Log                 tarch::multicore::TaskEnumerator::_log( "tarch::multicore::TaskEnumerator" );


tarch::multicore::TaskEnumerator::TaskEnumerator():
  _enumeratorNumber(_enumerator.size()) {
  _enumerator.push_back( new Enumerator() );

  const int newMax = std::numeric_limits<int>::max() / _enumerator.size() - 2;
  for (auto& p: _enumerator) {
    p->setMax(newMax);
  }
}


void tarch::multicore::TaskEnumerator::reset() {
  _enumerator.at(_enumeratorNumber)->reset();
}


int tarch::multicore::TaskEnumerator::size() const {
  return _enumerator.at(_enumeratorNumber)->size();
}


int tarch::multicore::TaskEnumerator::getNumber() {
  return _enumerator.at(_enumeratorNumber)->getNumber();
}


void tarch::multicore::TaskEnumerator::releaseNumber( int number ) {
  _enumerator.at(_enumeratorNumber)->releaseNumber(number);
}


std::string tarch::multicore::TaskEnumerator::toString() const {
  std::ostringstream msg;
  msg << "{";
  for (auto& p: _enumerator) {
    msg << p->toString();
  }
  msg << "}";
  return msg.str();
}
