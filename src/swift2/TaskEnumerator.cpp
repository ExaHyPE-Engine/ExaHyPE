#include "TaskEnumerator.h"

#include <bitset>

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"


std::map< int, tarch::multicore::BooleanSemaphore* >  swift2::TaskEnumerator::_resources;
swift2::TaskEnumerator::ResourcesSemaphore            swift2::TaskEnumerator::_resourcesSemaphore;



void swift2::TaskEnumerator::insertNumber(int number) {
  ResourcesSemaphore::Lock _lock( _resourcesSemaphore, ResourcesSemaphore::Lock::Write );
  if (number!=tarch::Enumerator::NoNumber and _resources.count(number)==0 ) {
    _resources.insert( {number, new tarch::multicore::BooleanSemaphore()} );
  }
  assertionEquals1( _resources.count(number), 1, number );
}


void swift2::TaskEnumerator::insertNumbers(const tarch::la::Vector<TwoPowerD,int>& numbers) {
  std::bitset<TwoPowerD> entryMissing = 0;

  {
    ResourcesSemaphore::Lock _lock( _resourcesSemaphore, ResourcesSemaphore::Lock::Read );
    for (int i=0; i<TwoPowerD; i++) {
      entryMissing[i] = numbers[i]!=tarch::Enumerator::NoNumber and _resources.count(numbers[i])==0;
    }
  }

  if ( entryMissing.any() ) {
    for (int i=0; i<TwoPowerD; i++) {
      insertNumber( numbers[i] );
    }
  }
}


void swift2::TaskEnumerator::lockResources( const tarch::la::Vector<TwoPowerD,int>& numbers ) {
  std::bitset<TwoPowerD> gotAllLocks = 0;

  for (int i=0; i<TwoPowerD; i++) {
    assertion2( numbers[i]==tarch::Enumerator::NoNumber or numbers[i]>=0, i, numbers[i] );
  }

  insertNumbers(numbers);

  while (not gotAllLocks.all()) {
    ResourcesSemaphore::Lock _lock( _resourcesSemaphore, ResourcesSemaphore::Lock::Read );
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
          assertionEquals2( _resources.count(numbers[i]), 1, i, numbers[i] );
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


void swift2::TaskEnumerator::unlockResources( const tarch::la::Vector<TwoPowerD,int>& numbers ) {
  for (int i=0; i<TwoPowerD; i++) {
    if (numbers[i] != tarch::Enumerator::NoNumber) {
      ResourcesSemaphore::Lock _lock( _resourcesSemaphore, ResourcesSemaphore::Lock::Read );
      assertion1( _resources.count(numbers[i])==1, numbers[i] );
      tarch::multicore::Lock lock(
        *_resources.at( numbers[i] ),
        false, false
      );
      lock.free();
    }
  }
}
