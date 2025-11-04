#include "Copy.h"
#include "tarch/Assertions.h"

#include <cmath>
#include <algorithm>


void toolbox::blockstructured::copyUnknowns(
  int                                numberOfDoFsPerAxisInPatch,
  const double* NOALIAS         source,
  int                                sourceHalo,
  double* NOALIAS               dest,
  int                                destHalo,
  int                                unknowns
) {
  #if DIMENSIONS==2
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * unknowns + unknown;
    int destIndexLinearised   = (x + destHalo + (y+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * unknowns + unknown;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #elif DIMENSIONS==3
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int z=0; z<numberOfDoFsPerAxisInPatch; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) + (z+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * unknowns + unknown;
    int destIndexLinearised   = (x + destHalo   + (y+destHalo)  *(numberOfDoFsPerAxisInPatch+2*destHalo)   + (z+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * unknowns + unknown;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #else
    #error DIMENSIONS not supported
  #endif
}


void toolbox::blockstructured::copyUnknown(
  int                                numberOfDoFsPerAxisInPatch,
  const double* NOALIAS         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* NOALIAS               dest,
  int                                destIndex,
  int                                destUnknowns,
  int                                destHalo
) {
  #if DIMENSIONS==2
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo + (y+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #elif DIMENSIONS==3
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int z=0; z<numberOfDoFsPerAxisInPatch; z++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) + (z+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo   + (y+destHalo)  *(numberOfDoFsPerAxisInPatch+2*destHalo)   + (z+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #else
    #error DIMENSIONS not supported
  #endif
}



double toolbox::blockstructured::copyUnknownAndComputeMaxDifference(
  int                                numberOfDoFsPerAxisInPatch,
  const double* NOALIAS         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* NOALIAS               dest,
  int                                destIndex,
  int                                destUnknowns,
  int                                destHalo
) {
  double result = 0.0;
  #if DIMENSIONS==2
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo + (y+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    double delta = std::abs( dest[destIndexLinearised] - source[sourceIndexLinearised] );
    dest[destIndexLinearised] = source[sourceIndexLinearised];
    result = std::max( result, delta );
  }
  #elif DIMENSIONS==3
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int z=0; z<numberOfDoFsPerAxisInPatch; z++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) + (z+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo   + (y+destHalo)  *(numberOfDoFsPerAxisInPatch+2*destHalo)   + (z+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    double delta = std::abs( dest[destIndexLinearised] - source[sourceIndexLinearised] );
    dest[destIndexLinearised] = source[sourceIndexLinearised];
    result = std::max( result, delta );
  }
  #else
    #error DIMENSIONS not supported
  #endif
  return result;
}


