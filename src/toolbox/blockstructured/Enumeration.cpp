#include "Enumeration.h"


int toolbox::blockstructured::serialisePatchIndexInOverlap(
  const tarch::la::Vector<Dimensions,int>& overlapCell,
  int                                      numberOfDoFsPerAxisInPatch,
  int                                      overlap,
  int                                      normal
) {
  assertion4(normal>=0,         overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
  assertion4(normal<Dimensions, overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);

  int base   = 1;
  int result = 0;
  for (int d=0; d<Dimensions; d++) {
    result += overlapCell(d) * base;
    if (d==normal) {
      assertion4(overlapCell(d)>=0, overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
      assertion4(overlapCell(d)<2,  overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
      base *= overlap*2;
    }
    else {
      assertion4(overlapCell(d)>=0,                          overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
      assertion4(overlapCell(d)<numberOfDoFsPerAxisInPatch,  overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
      base *= numberOfDoFsPerAxisInPatch;
    }
  }
  return result;
}

