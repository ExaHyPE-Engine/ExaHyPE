#include "PatchUtils.h"

#include "tarch/Assertions.h"
#include "tarch/la/la.h"



tarch::la::Vector<2,double>  exahype2::getVolumeSize(
  const tarch::la::Vector<2,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  tarch::la::Vector<2,double> volumeSize;

  for (int d=0; d<2; d++) {
    assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );
    volumeSize(d) = h(d)/numberOfVolumesPerAxisInPatch;
  }

  return volumeSize;
}


tarch::la::Vector<3,double>  exahype2::getVolumeSize(
  const tarch::la::Vector<3,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  tarch::la::Vector<3,double> volumeSize;

  for (int d=0; d<3; d++) {
    volumeSize(d) = h(d)/numberOfVolumesPerAxisInPatch;
  }

  return volumeSize;
}


double  exahype2::getVolumeLength(
  const tarch::la::Vector<2,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  return h(0)/numberOfVolumesPerAxisInPatch;
}


double  exahype2::getVolumeLength(
  const tarch::la::Vector<3,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );

  return h(0)/numberOfVolumesPerAxisInPatch;
}


tarch::la::Vector<2,double>  exahype2::getVolumeCentre(
  const tarch::la::Vector<2,double>&  x,
  const tarch::la::Vector<2,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch,
  const tarch::la::Vector<2,int>&     index
) {
  tarch::la::Vector<2,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<2,double> result     = x - 0.5 * h + 0.5 * volumeSize;
  for (int d=0; d<2; d++) {
    result(d) += index(d) * volumeSize(d);
  }
  return result;
}


tarch::la::Vector<3,double>  exahype2::getVolumeCentre(
  const tarch::la::Vector<3,double>&  x,
  const tarch::la::Vector<3,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch,
  const tarch::la::Vector<3,int>&     index
) {
  tarch::la::Vector<3,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<3,double> result     = x - 0.5 * h + 0.5 * volumeSize;
  for (int d=0; d<3; d++) {
    result(d) += index(d) * volumeSize(d);
  }
  return result;
}
