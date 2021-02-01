#include "Loop.h"


namespace {
  int dLinearisedNotOptimised( const tarch::la::Vector<Dimensions,int> & counter, int max ) {
    int result = counter(Dimensions - 1);
    assertion2(counter(Dimensions - 1) >= 0, counter, max);
    assertion2(counter(Dimensions - 1) < max, counter, max);
    for (int d = Dimensions - 2; d >= 0; d--) {
      assertion2(counter(d) >= 0, counter(d), max);
      assertion2(counter(d) < max, counter(d), max);
      result = result * max + counter(d);
    }
    return result;
  }


  tarch::la::Vector<Dimensions,int> dDelinearisedNotOptimised(int value, int max ) {
    assertion2(value >= 0, value, max);

    tarch::la::Vector<Dimensions,int> result(0);
    for (int d=Dimensions-1; d>=0; d--) {
      result(d)  = value /  tarch::la::aPowI(d,max);
      value     -= result(d) * tarch::la::aPowI(d,max);
    }
    return result;
  }


  #ifdef DloopOptimiseAggressive
  //const int MaxMax                  = 4;
  //const int MaxIndexOfLinearization = FOUR_POWER_D;
  const int MaxMax                  = 5;
  const int MaxIndexOfLinearization = FIVE_POWER_D;
  int       LookupTableDLinearised[MaxIndexOfLinearization*(MaxMax-1)];
  tarch::la::Vector<Dimensions,int> LookupTableDDeLinearised[MaxIndexOfLinearization*(MaxMax-1)];

  int getKeyForDLinearised(const tarch::la::Vector<Dimensions,int>& counter, int max) {
    int result = 0;
    int key    = 1;

    for (int d=0; d<Dimensions; d++) {
      result += key * counter(d);
      key    *= MaxMax;
    }

    assertionEquals( key, MaxIndexOfLinearization );
    // Nothing to be held for max=0 and max=1, so decrement the offset
    result += (max-2)*MaxIndexOfLinearization;

    return result;
  }


  int getKeyForDDeLinearised(int value, int max) {
    // Nothing to be held for max=0 and max=1, so decrement the offset
    return value + (max-2)*MaxIndexOfLinearization;
  }
  #endif
}


void peano4::utils::setupLookupTableForDLinearised() {
  #ifdef DloopOptimiseAggressive
  for (int i=2; i<=MaxMax; i++) {
    dfor(k,i) {
      LookupTableDLinearised[getKeyForDLinearised(k,i)] = dLinearisedNotOptimised(k,i);
    }
  }
  #endif
}


void peano4::utils::setupLookupTableForDDelinearised() {
  #ifdef DloopOptimiseAggressive
  for (int value=0; value<MaxIndexOfLinearization; value++) {
    for (int i=2; i<=MaxMax; i++) {
      LookupTableDDeLinearised[getKeyForDDeLinearised(value,i)] = dDelinearisedNotOptimised(value,i);
    }
  }
  #endif
}


int peano4::utils::dLinearisedWithoutLookup( const tarch::la::Vector<Dimensions,int>& counter, int max ) {
	return dLinearisedNotOptimised(counter,max);
}


int peano4::utils::dLinearised( const std::bitset<Dimensions>& counter ) {
  int result = 0;
  int base   = 1;
  for (int d=0; d<Dimensions; d++) {
	result += (counter[d] ? 1 : 0) * base;
    base   *= 2;
  }
  return result;
}


int peano4::utils::dLinearised( const tarch::la::Vector<Dimensions,int>& counter, int max ) {
  #ifdef DloopOptimiseAggressive
    assertionEquals3(LookupTableDLinearised[getKeyForDLinearised(counter,max)], dLinearisedNotOptimised(counter,max), counter,max,getKeyForDLinearised(counter,max));
    return LookupTableDLinearised[getKeyForDLinearised(counter,max)];
  #else
    return dLinearisedNotOptimised(counter,max);
  #endif
}


int peano4::utils::d2Linearised( const tarch::la::Vector<2,int>& counter, int max ) {
  int result = counter(2 - 1);
  assertion2(counter(2 - 1) >= 0, counter, max);
  assertion2(counter(2 - 1) < max, counter, max);
  for (int d = 2 - 2; d >= 0; d--) {
    assertion3(counter(d) >= 0, counter, d, max);
    assertion3(counter(d) < max, counter, d, max);
    result = result * max + counter(d);
  }
  return result;
}


int peano4::utils::d3Linearised( const tarch::la::Vector<3,int>& counter, int max ) {
  int result = counter(3 - 1);
  assertion2(counter(3 - 1) >= 0, counter, max);
  assertion2(counter(3 - 1) < max, counter, max);
  for (int d = 3 - 2; d >= 0; d--) {
    assertion2(counter(d) >= 0, counter(d), max);
    assertion2(counter(d) < max, counter(d), max);
    result = result * max + counter(d);
  }
  return result;
}


tarch::la::Vector<Dimensions,int> peano4::utils::dDelinearisedWithoutLookup(int value, int max) {
  return dDelinearisedNotOptimised(value, max);
}

tarch::la::Vector<Dimensions,int> peano4::utils::dDelinearised(int value, int max ) {
  #ifdef DloopOptimiseAggressive
    for (int d=0; d<Dimensions; d++) {
      assertionEquals2(LookupTableDDeLinearised[getKeyForDDeLinearised(value,max)](d), dDelinearisedNotOptimised(value,max)(d),value,max);
    }
    return LookupTableDDeLinearised[getKeyForDDeLinearised(value,max)];
  #else
    return dDelinearisedNotOptimised(value,max);
  #endif
}


void peano4::utils::dInc(tarch::la::Vector<Dimensions,int>& counter, int max) {
  counter(0)++;
  for (int i=0; i<Dimensions-1; i++) {
    if ( counter(i) >= max ) {
      counter(i) = 0;
      counter(i+1)++;
    }
  }
}


void peano4::utils::dDec(tarch::la::Vector<Dimensions,int>& counter, int max) {
  for (int i = 0; i < Dimensions - 1; i++) {
    assertion(counter(i) >= 0);
  }

  counter(0)--;
  for (int i=0; i<Dimensions-1; i++) {
    if ( counter(i) < 0 ) {
      counter(i) = max-1;
      counter(i+1)--;
    }
  }
}


void peano4::utils::dInc(tarch::la::Vector<Dimensions,int>& counter, const tarch::la::Vector<Dimensions,int>& max) {
  counter(0)++;
  for (int i=0; i<Dimensions-1; i++) {
    if ( counter(i) >= max(i) ) {
      counter(i) = 0;
      counter(i+1)++;
    } else {
      break;
    }
  }
}

void peano4::utils::dIncByVector(tarch::la::Vector<Dimensions,int>& counter, int max, int increment) {
  counter(0) += increment;
   for (int i=0; i<Dimensions-1; i++) {
     if ( counter(i) >= max ) {
       counter(i) %= max;
       counter(i+1) += increment;
     } else {
       break;
     }
   }
}


void peano4::utils::dIncByScalar(tarch::la::Vector<Dimensions,int>& counter, int max, int increment) {
  counter(0) += increment;
  for (int i=0; i<Dimensions-1; i++) {
    while ( counter(i) >= max ) {
      counter(i) -= max;
      counter(i+1) ++;
    }
  }
}


void peano4::utils::dInc(tarch::la::Vector<Dimensions,int>& counter, int max, int doNotExamine) {
  assertion( doNotExamine<Dimensions);
  assertion( doNotExamine>=-1);

  if (doNotExamine!=0) {
    counter(0)++;
  }
  else {
    counter(1)++;
  }
  for (int i=0; i<Dimensions-1; i++) {
    if ( counter(i) >= max ) {
      counter(i) = 0;
      if ( doNotExamine != i+1) {
        counter(i+1)++;
      }
      else {
        if ( doNotExamine != Dimensions-1 ) {
          counter(i+2)++;
        }
        else {
          counter(Dimensions-1) = max;
        }
      }
    }
  }
}


void peano4::utils::dInc(tarch::la::Vector<Dimensions,int>& counter, int max, peano4::utils::LoopDirection& direction ) {
  for (int i=0; i<Dimensions; i++) {
    if ( direction[i] ) {
      counter(i)++;
      bool greater = counter(i) >= max;
      if (greater && (i==Dimensions-1)) {
        counter(i)=max;
        return;
      }
      else if (greater) {
        counter(i)--;
        direction[i] = !direction[i];
      }
      else return;
    }
    else {
      counter(i)--;
      bool smaller = counter(i) < 0;
      if (smaller && (i==Dimensions-1)) {
        counter(i)=max;
        return;
      }
      else if (smaller) {
        counter(i)++;
        direction[i] = !direction[i];
      }
      else return;
    }
  }
}


int peano4::utils::dCmp(const tarch::la::Vector<Dimensions,int>& counter, const tarch::la::Vector<Dimensions,int>& max) {
  int result = true;
  for (int i=0; i<Dimensions; i++) {
    assertion2( counter(i)<=max(i), counter,max );
    assertion2( counter(i)>=0,   counter,max );
    result &= (counter(i)<max(i));
  }
  return result;
}


int peano4::utils::dCmp(const tarch::la::Vector<Dimensions,int>& counter, int max) {
  int result = true;
  for (int i=0; i<Dimensions; i++) {
    assertion2( counter(i)<=max, counter,max );
    assertion2( counter(i)>=0,   counter,max );
    result &= (counter(i)<max);
  }
  return result;
}


bool peano4::utils::dCmpLinearOrder(const tarch::la::Vector<Dimensions,int>& counter, const tarch::la::Vector<Dimensions,int>& max) {
  for (int i = Dimensions - 1; i >= 0; i--) {
    if (counter(i) > max(i)) {
      return false;
    } else if (counter(i) < max(i)) {
      return true;
    }
  }
  return false;
}

tarch::la::Vector<Dimensions,int> peano4::utils::dStartVector() {
  tarch::la::Vector<Dimensions,int> result(0);
  return result;
}


tarch::la::Vector<Dimensions,int> peano4::utils::dStartVector(int dim, int value) {
  tarch::la::Vector<Dimensions,int> result(0);
  assertion2( dim >= 0, dim, value );
  assertion2( dim <  Dimensions, dim, value );
  assertion2( value >= 0, dim, value );
  result( dim ) = value;
  return result;
}


tarch::la::Vector<Dimensions,int> peano4::utils::dStartVector( int max, const peano4::utils::LoopDirection& direction ) {
  tarch::la::Vector<Dimensions,int> result;
  assertion2( max>0 && max<=3, max, direction );
  for (int i=0; i<Dimensions; i++) {
    if (direction[i]) {
      result(i) = 0;
    }
    else {
      result(i) = max-1;
    }
  }
  return result;
}
