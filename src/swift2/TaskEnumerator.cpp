#include "TaskEnumerator.h"


tarch::Enumerator swift2::TaskEnumerator::_globalEnumerator;


void swift2::TaskEnumerator::reset() {
  _globalEnumerator.reset();
}


int swift2::TaskEnumerator::getNumber() {
  return _globalEnumerator.getNumber();
}


void swift2::TaskEnumerator::releaseNumber( int value ) {
  _globalEnumerator.releaseNumber(value);
}


int swift2::TaskEnumerator::size() const {
  return _globalEnumerator.size();
}


std::string swift2::TaskEnumerator::toString() const {
  return _globalEnumerator.toString();
}

