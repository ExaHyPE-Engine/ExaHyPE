// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Riemann.h"


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesUnknowns(
    int numberOfVolumesPerAxisInPatch,
    int unknowns
  ) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns
  });
}


tarch::la::Vector<Dimensions + 2, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesUnknownsPlusAuxiliaryVariablesTimesPatches(
    int numberOfVolumesPerAxisInPatch,
    int unknowns,
    int auxiliaryVariables,
    int patches
  ) {
  return tarch::la::Vector<Dimensions + 2, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns + auxiliaryVariables, patches
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesPatches(int numberOfVolumesPerAxisInPatch, int patches) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<Dimensions + 2, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesUnknownsTimesPatches(
    int numberOfVolumesPerAxisInPatch,
    int unknowns,
    int patches
  ) {
  return tarch::la::Vector<Dimensions + 2, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns, patches
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesUnknownsPlusAuxiliaryVariables(
    int numberOfVolumesPerAxisInPatch,
    int unknowns,
    int auxiliaryVariables
  ) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns + auxiliaryVariables
  });
}


tarch::la::Vector<Dimensions, int> exahype2::fv::riemann::internal::
  rangeOverVolumesPlusHaloInXDirection(
    int  numberOfVolumesPerAxisInPatch,
    int  haloSize,
    bool extendInBothDirections
  ) {
  return tarch::la::Vector<Dimensions, int>({
    numberOfVolumesPerAxisInPatch + (extendInBothDirections ? 2 : 1) * haloSize,
      numberOfVolumesPerAxisInPatch
#if Dimensions == 3
      ,
      numberOfVolumesPerAxisInPatch
#endif
  });
}


tarch::la::Vector<Dimensions, int> exahype2::fv::riemann::internal::
  rangeOverVolumesPlusHaloInYDirection(
    int  numberOfVolumesPerAxisInPatch,
    int  haloSize,
    bool extendInBothDirections
  ) {
  return tarch::la::Vector<Dimensions, int>({
    numberOfVolumesPerAxisInPatch,
      numberOfVolumesPerAxisInPatch
        + (extendInBothDirections ? 2 : 1) * haloSize
#if Dimensions == 3
      ,
      numberOfVolumesPerAxisInPatch
#endif
  });
}


tarch::la::Vector<3, int> exahype2::fv::riemann::internal::
  rangeOverVolumesPlusHaloInZDirection(
    int  numberOfVolumesPerAxisInPatch,
    int  haloSize,
    bool extendInBothDirections
  ) {
  return tarch::la::Vector<3, int>(
    {numberOfVolumesPerAxisInPatch,
     numberOfVolumesPerAxisInPatch,
     numberOfVolumesPerAxisInPatch
       + (extendInBothDirections ? 2 : 1) * haloSize}
  );
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesPatchesPlusHaloInXDirection(
    int numberOfVolumesPerAxisInPatch,
    int haloSize,
    int patches
  ) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch + haloSize, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesPatchesPlusHaloInYDirection(
    int numberOfVolumesPerAxisInPatch,
    int haloSize,
    int patches
  ) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch + haloSize,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<4, int> exahype2::fv::riemann::internal::
  rangeOverVolumesTimesPatchesPlusHaloInZDirection(
    int numberOfVolumesPerAxisInPatch,
    int haloSize,
    int patches
  ) {
  return tarch::la::Vector<4, int>(
    {numberOfVolumesPerAxisInPatch,
     numberOfVolumesPerAxisInPatch,
     numberOfVolumesPerAxisInPatch + haloSize,
     patches}
  );
}
