#include "Copy.h"
#include "tarch/Assertions.h"


void toolbox::blockstructured::copyUnknown(
  int                                numberOfDoFsPerAxisInPatch,
  const double* __restrict__         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* __restrict__               dest,
  int                                destIndex,
  int                                destUnknowns,
  int                                destHalo
) {
  #if Dimensions==2
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo + (y+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
    assertion5( source[sourceIndexLinearised]>0.0, sourceIndexLinearised, x, y, sourceIndex, sourceUnknowns ); // @todo raus
  }
  #elif Dimensions==3
  for (int x=0; x<numberOfDoFsPerAxisInPatch; x++)
  for (int y=0; y<numberOfDoFsPerAxisInPatch; y++)
  for (int z=0; z<numberOfDoFsPerAxisInPatch; z++) {
    int sourceIndexLinearised = (x + sourceHalo + (y+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) + (z+sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo)*(numberOfDoFsPerAxisInPatch+2*sourceHalo) ) * sourceUnknowns + sourceIndex;
    int destIndexLinearised   = (x + destHalo   + (y+destHalo)  *(numberOfDoFsPerAxisInPatch+2*destHalo)   + (z+destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo)*(numberOfDoFsPerAxisInPatch+2*destHalo) ) * destUnknowns + destIndex;
    dest[destIndexLinearised] = source[sourceIndexLinearised];
  }
  #else
    #error Dimensions not supported
  #endif
}


