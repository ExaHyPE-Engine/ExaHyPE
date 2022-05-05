#include "Derivative.h"
#include "peano4/utils/Loop.h"

#include <iostream>


void toolbox::blockstructured::computeGradient(
  int                                numberOfDoFsPerAxisInPatch,
  const double* __restrict__         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* __restrict__               dest,
  const tarch::la::Vector<Dimensions,int>&   destIndex,
  int                                destUnknowns,
  int                                destHalo,
  const tarch::la::Vector<Dimensions,double>&  volumeH
) {
  dfor(k,numberOfDoFsPerAxisInPatch) {
    for (int d=0; d<Dimensions; d++) {
      tarch::la::Vector<Dimensions,int> sourceVolumeLeft  = k + tarch::la::Vector<Dimensions,int>(sourceHalo);
      tarch::la::Vector<Dimensions,int> sourceVolumeRight = k + tarch::la::Vector<Dimensions,int>(sourceHalo);
      tarch::la::Vector<Dimensions,int> destVolume        = k + tarch::la::Vector<Dimensions,int>(destHalo);

      if (k(d)<=numberOfDoFsPerAxisInPatch/2) {
        sourceVolumeRight(d)++;
      }
      else {
        sourceVolumeLeft(d)--;
      }

      int sourceVolumeLeftLinearised  = peano4::utils::dLinearised(sourceVolumeLeft,  numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int sourceVolumeRightLinearised = peano4::utils::dLinearised(sourceVolumeRight, numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int destVolumeLinearised        = peano4::utils::dLinearised(destVolume,        numberOfDoFsPerAxisInPatch+2*destHalo);

      double grad = ( source[sourceVolumeRightLinearised*sourceUnknowns+sourceIndex] - source[sourceVolumeLeftLinearised*sourceUnknowns+sourceIndex] ) / volumeH(d) / numberOfDoFsPerAxisInPatch;

      dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] = grad;
    }
  }
}


double toolbox::blockstructured::computeGradientAndReturnMaxDifference(
  int                                numberOfDoFsPerAxisInPatch,
  const double* __restrict__         source,
  int                                sourceIndex,
  int                                sourceUnknowns,
  int                                sourceHalo,
  double* __restrict__               dest,
  const tarch::la::Vector<Dimensions,int>&   destIndex,
  int                                destUnknowns,
  int                                destHalo,
  const tarch::la::Vector<Dimensions,double>&  volumeH
) {
  double result = 0.0;
  dfor(k,numberOfDoFsPerAxisInPatch) {
    for (int d=0; d<Dimensions; d++) {
      tarch::la::Vector<Dimensions,int> sourceVolumeLeft  = k + tarch::la::Vector<Dimensions,int>(sourceHalo);
      tarch::la::Vector<Dimensions,int> sourceVolumeRight = k + tarch::la::Vector<Dimensions,int>(sourceHalo);
      tarch::la::Vector<Dimensions,int> destVolume        = k + tarch::la::Vector<Dimensions,int>(destHalo);

      if (k(d)<=numberOfDoFsPerAxisInPatch/2) {
        sourceVolumeRight(d)++;
      }
      else {
        sourceVolumeLeft(d)--;
      }

      int sourceVolumeLeftLinearised  = peano4::utils::dLinearised(sourceVolumeLeft,  numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int sourceVolumeRightLinearised = peano4::utils::dLinearised(sourceVolumeRight, numberOfDoFsPerAxisInPatch+2*sourceHalo);
      int destVolumeLinearised        = peano4::utils::dLinearised(destVolume,        numberOfDoFsPerAxisInPatch+2*destHalo);

      double grad = ( source[sourceVolumeRightLinearised*sourceUnknowns+sourceIndex] - source[sourceVolumeLeftLinearised*sourceUnknowns+sourceIndex] ) / volumeH(d) / numberOfDoFsPerAxisInPatch;

      double delta = std::abs( dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] - grad );
      dest[ destVolumeLinearised*destUnknowns+destIndex(d) ] = grad;
      result = std::max(result,delta);
    }
  }
  return result;
}
