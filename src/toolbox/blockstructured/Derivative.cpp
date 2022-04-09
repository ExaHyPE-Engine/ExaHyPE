#include "Derivative.h"


void toolbox::blockstructured::computeGradient(
  int                                numberOfDoFsPerAxisInPatch,
  const double* __restrict__         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* __restrict__               dest,
  int                                destIndex[Dimensions],
  int                                destUnknowns,
  int                                destHalo
) {
  #if Dimensions==2
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++) {
    int sourceIndexLinearised = (x + y*numberOfDoFsPerAxisInPatch) * sourceUnknowns;
    int destIndexLinearised   = (x + y*numberOfDoFsPerAxisInPatch) * destUnknowns;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #elif Dimensions==3
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int z=0; z<numberOfDoFsPerAxisInPatch; z++) {
    int sourceIndexLinearised = (x + y*numberOfDoFsPerAxisInPatch) * sourceUnknowns;
    int destIndexLinearised   = (x + y*numberOfDoFsPerAxisInPatch) * destUnknowns;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #else
    #error Dimensions not supported
  #endif
}


