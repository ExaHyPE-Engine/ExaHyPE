#include "PatchUtils.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/Assertions.h"
#include "tarch/la/la.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"



/*
double  exahype2::fd::getGridCellLength(
  const tarch::la::Vector<2,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  return exahype2::fv::getVolumeLength(h,numberOfVolumesPerAxisInPatch);
}


double  exahype2::fd::getGridCellLength(
  const tarch::la::Vector<3,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  return exahype2::fv::getVolumeLength(h,numberOfVolumesPerAxisInPatch);
}
*/


std::string exahype2::fd::plotGridCell(
  const double* __restrict__ Q,
  int    unknowns
) {
  return exahype2::fv::plotVolume(Q,unknowns);
}


std::string exahype2::fd::plotPatch(
  const double* __restrict__ Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize,
  bool   prettyPrint
) {
  return exahype2::fv::plotPatch(
    Q,
    unknowns,
    auxiliaryVariables,
    numberOfVolumesPerAxisInPatch,
    haloSize,
    prettyPrint
  );
}


std::string exahype2::fd::plotPatchOverlap(
  const double* __restrict__ Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfGridCellsPerPatchPerAxis,
  int    haloSize,
  int    normal,
  bool   prettyPrint
) {
  return exahype2::fv::plotPatchOverlap(
      Q,
      unknowns,
      auxiliaryVariables,
      numberOfGridCellsPerPatchPerAxis,
      haloSize,
      normal,
      prettyPrint
    );
}


void exahype2::fd::validatePatch (
  const double *__restrict__ Q, int unknowns,
  int auxiliaryVariables,
  int numberOfVolumesPerAxisInPatch, int haloSize,
  const std::string &location,
  bool   triggerNonCriticalAssertion,
  double* minValues,
  double* maxValues
) {
  exahype2::fv::validatePatch (
    Q, unknowns, auxiliaryVariables, numberOfVolumesPerAxisInPatch, haloSize,
    location, triggerNonCriticalAssertion, minValues, maxValues
  );
}



