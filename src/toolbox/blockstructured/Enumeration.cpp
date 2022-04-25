#include "Enumeration.h"


int toolbox::blockstructured::serialiseVoxelIndexInOverlap(
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
      assertion4(overlapCell(d)>=0,         overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
      assertion4(overlapCell(d)<2*overlap,  overlapCell, numberOfDoFsPerAxisInPatch, overlap, normal);
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


int toolbox::blockstructured::serialisePatchIndexInOverlap(
  const tarch::la::Vector<Dimensions,int>& patchIndex,
  int                                      normal
) {
  assertion2(normal>=0,         patchIndex, normal);
  assertion2(normal<Dimensions, patchIndex, normal);

  int base   = 1;
  int result = 0;
  for (int d=0; d<Dimensions; d++) {
    if (d!=normal) {
      result += patchIndex(d) * base;
      assertion2(patchIndex(d)>=0, patchIndex, normal);
      assertion2(patchIndex(d)<3,  patchIndex, normal);
      base *= 3;
    }
  }
  return result;
}


int toolbox::blockstructured::serialiseMarkerIn3x3PatchAssembly(
  const tarch::la::Vector<Dimensions,int>& markerIndex,
  int                                      numberOfDoFsPerAxisInPatch
) {
  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
  return (markerIndex(0) + markerIndex(1)*3) * patchSize;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
  return (markerIndex(0) + markerIndex(1)*3 + markerIndex(2)*3*3) * patchSize;
  #endif
}

