// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "rusanov.h"


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesPatches(int numberOfVolumesPerAxisInPatch, int patches) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<Dimensions + 2, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesUnknownsPlusAuxiliaryVariablesTimesPatches(
  int numberOfVolumesPerAxisInPatch, int unknowns, int auxiliaryVariables, int patches
) {
  return tarch::la::Vector<Dimensions + 2, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns + auxiliaryVariables, patches
  });
}

tarch::la::Vector<Dimensions + 2, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesUnknownsTimesPatches(int numberOfVolumesPerAxisInPatch, int unknowns, int patches) {
  return tarch::la::Vector<Dimensions + 2, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns, patches
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesUnknownsPlusAuxiliaryVariables(
  int numberOfVolumesPerAxisInPatch, int unknowns, int auxiliaryVariables
) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns + auxiliaryVariables
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesUnknowns(int numberOfVolumesPerAxisInPatch, int unknowns) {
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      unknowns
  });
}


tarch::la::Vector<Dimensions, int> exahype2::fv::rusanov::internal::rangeOverVolumesPlusHaloInXDirection(int numberOfVolumesPerAxisInPatch) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<Dimensions, int>({
    numberOfVolumesPerAxisInPatch + 2 * haloSize, numberOfVolumesPerAxisInPatch
#if Dimensions == 3
      ,
      numberOfVolumesPerAxisInPatch
#endif
  });
}


tarch::la::Vector<Dimensions, int> exahype2::fv::rusanov::internal::rangeOverVolumesPlusHaloInYDirection(int numberOfVolumesPerAxisInPatch) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<Dimensions, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch + 2 * haloSize
#if Dimensions == 3
      ,
      numberOfVolumesPerAxisInPatch
#endif
  });
}


tarch::la::Vector<3, int> exahype2::fv::rusanov::internal::rangeOverVolumesPlusHaloInZDirection(int numberOfVolumesPerAxisInPatch) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<3, int>({numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch + 2 * haloSize});
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesPatchesPlusHaloInXDirection(int numberOfVolumesPerAxisInPatch, int patches) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch + 2 * haloSize, numberOfVolumesPerAxisInPatch,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<Dimensions + 1, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesPatchesPlusHaloInYDirection(int numberOfVolumesPerAxisInPatch, int patches) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<Dimensions + 1, int>({
    numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch + 2 * haloSize,
#if Dimensions == 3
      numberOfVolumesPerAxisInPatch,
#endif
      patches
  });
}


tarch::la::Vector<4, int> exahype2::fv::rusanov::internal::rangeOverVolumesTimesPatchesPlusHaloInZDirection(int numberOfVolumesPerAxisInPatch, int patches) {
  constexpr int haloSize = 1;
  return tarch::la::Vector<4, int>({numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch, numberOfVolumesPerAxisInPatch + 2 * haloSize, patches});
}
