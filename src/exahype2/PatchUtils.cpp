#include "PatchUtils.h"

#include "tarch/Assertions.h"
#include "tarch/la/la.h"



tarch::la::Vector<Dimensions,double>  exahype2::getVolumeSize(
  const tarch::la::Vector<Dimensions,double>&  h,
  int                                          numberOfVolumesPerAxisInPatch
) {
  tarch::la::Vector<Dimensions,double> volumeSize;

  for (int d=0; d<Dimensions; d++) {
    assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );
    volumeSize(d) = h(d)/numberOfVolumesPerAxisInPatch;
  }

  return volumeSize;
}


double  exahype2::getVolumeLength(
  const tarch::la::Vector<Dimensions,double>&  h,
  int                                          numberOfVolumesPerAxisInPatch
) {
  #if !defined(GPUOffloading)
  assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );
  #endif

  return h(0)/numberOfVolumesPerAxisInPatch;
}


tarch::la::Vector<Dimensions,double>  exahype2::getVolumeCentre(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  int                                          numberOfVolumesPerAxisInPatch,
  const tarch::la::Vector<Dimensions,int>&     index
) {
  tarch::la::Vector<Dimensions,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<Dimensions,double> result     = x - 0.5 * h + 0.5 * volumeSize;
  for (int d=0; d<Dimensions; d++) {
    result(d) += index(d) * volumeSize(d);
  }
  return result;
}


