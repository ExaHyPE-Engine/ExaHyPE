// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Linearised.h"

#include "Loop.h"

int                                peano4::utils::lookupTableDLinearised[MaxIndexOfLinearisation * (MAX_TABLE - 1)];
tarch::la::Vector<DIMENSIONS, int> peano4::utils::lookupTableDDeLinearised[MaxIndexOfLinearisation * (MAX_TABLE - 1)];

void peano4::utils::setupLookupTableForDLinearised() {
  for (int max = 2; max <= MAX_TABLE; max++) {
    dfor(k, max) { lookupTableDLinearised[getKeyForDLinearisedLookup(k, max)] = dLinearisedWithoutLookup(k, max); }
  }
}

void peano4::utils::setupLookupTableForDDelinearised() {
  for (int max = 2; max <= MAX_TABLE; max++) {
    for (int value = 0; value < tarch::la::aPowI(DIMENSIONS, max); value++) {
      int key = peano4::utils::getKeyForDDeLinearisedLookup(value, max);
#if !defined(WITH_GPU)
      assertion3(key >= 0 && key < MaxIndexOfLinearisation * (MAX_TABLE - 1), key, max, MaxIndexOfLinearisation);
#endif
      lookupTableDDeLinearised[key] = dDelinearisedWithoutLookup(value, max);
    }
  }
}
