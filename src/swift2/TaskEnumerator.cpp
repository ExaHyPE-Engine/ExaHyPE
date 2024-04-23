#include "TaskEnumerator.h"

#include <bitset>

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"


std::map< int, tarch::multicore::BooleanSemaphore* >  swift2::TaskEnumerator::_resources;
tarch::Enumerator swift2::TaskEnumerator::_globalEnumerator;


void swift2::TaskEnumerator::reset() {
  _globalEnumerator.reset();
}


int swift2::TaskEnumerator::getNumber() {
  int result = _globalEnumerator.getNumber();
  if ( _resources.count(result)==0 ) {
    _resources.insert( {result, new tarch::multicore::BooleanSemaphore()} );
  }
  return result;
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


void swift2::TaskEnumerator::lockResources( const int numbers[TwoPowerD] ) {
  std::bitset<TwoPowerD> gotAllLocks = 0;

  while (not gotAllLocks.all()) {
    int counter = 0;
    while (counter<TwoPowerD) {
      assertion1( _resources.count(numbers[counter])==1, counter );
      tarch::multicore::Lock lock(
        *_resources.at( numbers[counter] ),
        false, false
      );
      gotAllLocks[counter] = lock.tryLock();
      if (gotAllLocks[counter]) {
        counter++;
      }
      else {
        counter = TwoPowerD;
      }
    }

    if (not gotAllLocks.all()) {
      for (int i=0; i<TwoPowerD; i++) {
        if (gotAllLocks[i]) {
          gotAllLocks[i] = false;
          tarch::multicore::Lock lock(
            *_resources.at( numbers[i] ),
            false, false
          );
          lock.free();
        }
      }

      tarch::multicore::Core::getInstance().yield();
    }
  }
}


void swift2::TaskEnumerator::unlockResources( const int numbers[TwoPowerD] ) {
  for (int i=0; i<TwoPowerD; i++) {
    assertion1( _resources.count(numbers[i])==1, numbers[i] );
    tarch::multicore::Lock lock(
      *_resources.at( numbers[i] ),
      false, false
    );
    lock.free();
  }
}
