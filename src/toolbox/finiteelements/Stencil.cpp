#include "Stencil.h"


int toolbox::finiteelements::getStencilEntryLinearisedIndex( const tarch::la::Vector<Dimensions,int>  stencilEntry) {
  int result = 0;
  int base   = 1;

  for (int d=0; d<Dimensions; d++) {
    result += stencilEntry(d) * base;
    base *= 3;
  }

  return result;
}
