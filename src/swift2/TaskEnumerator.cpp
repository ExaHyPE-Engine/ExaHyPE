#include "TaskEnumerator.h"

#include <bitset>

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"


std::map< int, tarch::multicore::BooleanSemaphore* >  swift2::TaskEnumerator::_resources;
tarch::Enumerator                                     swift2::TaskEnumerator::_globalEnumerator;
tarch::multicore::BooleanSemaphore                    swift2::TaskEnumerator::_resourcesSemaphore;


void swift2::TaskEnumerator::reset() {
  _globalEnumerator.reset();
}


int swift2::TaskEnumerator::getNumber() {
  int result = _globalEnumerator.getNumber();
  tarch::multicore::Lock _lock( _resourcesSemaphore );
  if ( _resources.count(result)==0 ) {
    _resources.insert( {result, new tarch::multicore::BooleanSemaphore()} );
  }
  assertionEquals1( _resources.count(result), 1, result );
  return result;
}


void swift2::TaskEnumerator::releaseNumber( int value ) {
  tarch::multicore::Lock _lock( _resourcesSemaphore );
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
    tarch::multicore::Lock _lock( _resourcesSemaphore );
    int counter = 0;
    while (counter<TwoPowerD) {
      if (numbers[counter] == tarch::Enumerator::NoNumber) {
        gotAllLocks[counter] = true;
      }
      else {
        assertionEquals2( _resources.count(numbers[counter]), 1, counter, numbers[counter] );
        tarch::multicore::Lock lock(
          *_resources.at( numbers[counter] ),
          false, false
        );
        gotAllLocks[counter] = lock.tryLock();
      }
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
    } // Hasn't worked, so we have released all the locks and did yield.
      // While loop fill try again next.
  }
}


void swift2::TaskEnumerator::unlockResources( const int numbers[TwoPowerD] ) {
  for (int i=0; i<TwoPowerD; i++) {
    if (numbers[i] != tarch::Enumerator::NoNumber) {
      tarch::multicore::Lock _lock( _resourcesSemaphore );
      assertion1( _resources.count(numbers[i])==1, numbers[i] );
      tarch::multicore::Lock lock(
        *_resources.at( numbers[i] ),
        false, false
      );
      lock.free();
    }
  }
}
