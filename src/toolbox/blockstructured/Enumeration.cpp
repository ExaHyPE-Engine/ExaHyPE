#include "Enumeration.h"


int toolbox::blockstructured::serialisePatchIndexInOverlap(
  const tarch::la::Vector<Dimensions,int>& overlapCell,
  int                                      numberOfDoFsPerAxisInPatch,
  int                                      overlap,
  int                                      normal
) {
  int base   = 1;
  int result = 0;
  for (int d=0; d<Dimensions; d++) {
    result += overlapCell(d) * base;
    if (d==normal) {
      base *= overlap*2;
    }
    else {
      base *= numberOfDoFsPerAxisInPatch;
    }
  }
  return result;
}

