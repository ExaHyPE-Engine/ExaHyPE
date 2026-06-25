#include "Derivative.h"
#include "peano4/Loop.h"

#include <iostream>


void toolbox::blockstructured::computeGradient(
  int                                numberOfDoFsPerAxisInPatch,
  const double* NOALIAS         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* NOALIAS               dest,
  const tarch::la::Vector<DIMENSIONS,int>&   destIndex,
  int                                destUnknowns,
  int                                destHalo,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH
) {
  dfor(k,numberOfDoFsPerAxisInPatch) {
    for (int d=0; d<DIMENSIONS; d++) {
      tarch::la::Vector<DIMENSIONS,int> sourceVolumeLeft  = k + tarch::la::Vector<DIMENSIONS,int>(sourceHalo);
      tarch::la::Vector<DIMENSIONS,int> sourceVolumeRight = k + tarch::la::Vector<DIMENSIONS,int>(sourceHalo);
      tarch::la::Vector<DIMENSIONS,int> destVolume        = k + tarch::la::Vector<DIMENSIONS,int>(destHalo);

      if (k(d)<=numberOfDoFsPerAxisInPatch/2) {
        sourceVolumeRight(d)++;
      }
      else {
        sourceVolumeLeft(d)--;
      }

      int sourceVolumeLeftLinearised  = peano4::dLinearised(sourceVolumeLeft,  numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int sourceVolumeRightLinearised = peano4::dLinearised(sourceVolumeRight, numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int destVolumeLinearised        = peano4::dLinearised(destVolume,        numberOfDoFsPerAxisInPatch+2*destHalo);

      double grad = ( source[sourceVolumeRightLinearised*sourceUnknowns+sourceIndex] - source[sourceVolumeLeftLinearised*sourceUnknowns+sourceIndex] ) / volumeH(d) / numberOfDoFsPerAxisInPatch;

      dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] = grad;
    }
  }
}


double toolbox::blockstructured::computeGradientAndReturnMaxDifference(
  int                                numberOfDoFsPerAxisInPatch,
  const double* NOALIAS         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* NOALIAS               dest,
  const tarch::la::Vector<DIMENSIONS,int>&   destIndex,
  int                                destUnknowns,
  int                                destHalo,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH
) {
  double result = 0.0;
  dfor(k,numberOfDoFsPerAxisInPatch) {
    for (int d=0; d<DIMENSIONS; d++) {
      tarch::la::Vector<DIMENSIONS,int> sourceVolumeLeft  = k + tarch::la::Vector<DIMENSIONS,int>(sourceHalo);
      tarch::la::Vector<DIMENSIONS,int> sourceVolumeRight = k + tarch::la::Vector<DIMENSIONS,int>(sourceHalo);
      tarch::la::Vector<DIMENSIONS,int> destVolume        = k + tarch::la::Vector<DIMENSIONS,int>(destHalo);

      if (k(d)<=numberOfDoFsPerAxisInPatch/2) {
        sourceVolumeRight(d)++;
      }
      else {
        sourceVolumeLeft(d)--;
      }

      int sourceVolumeLeftLinearised  = peano4::dLinearised(sourceVolumeLeft,  numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int sourceVolumeRightLinearised = peano4::dLinearised(sourceVolumeRight, numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int destVolumeLinearised        = peano4::dLinearised(destVolume,        numberOfDoFsPerAxisInPatch+2*destHalo);

      double grad = ( source[sourceVolumeRightLinearised*sourceUnknowns+sourceIndex] - source[sourceVolumeLeftLinearised*sourceUnknowns+sourceIndex] ) / volumeH(d) / numberOfDoFsPerAxisInPatch;

      double delta = std::abs( dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] - grad );
      dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] = grad;
      result = std::max(result,delta);
    }
  }
  return result;
}
