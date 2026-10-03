// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HaloProfileTestData.h"

#include "exahype2/fv/FiniteVolume.h"

double
exahype2::fv::tests::getLinearHaloProfile(
  const int    layer,
  const double firstPosition,
  const double secondPosition,
  const int    unknown
) {
  return 1.0 + 10.0 * layer + 100.0 * unknown + 2.0 * firstPosition - 3.0 * secondPosition;
}

/**
 * Returns the position of a fine volume of a subface in coarse volume units.
 */
double
exahype2::fv::tests::getFinePosition(const int volume, const int subface) {
  return (volume + HaloProfileSetup::DofsPerAxis * subface + 0.5) / HaloProfileSetup::SubdivisionFactor - 0.5;
}

std::vector<double>
exahype2::fv::tests::getFineFaceWithProfile(const int normal, const int firstSubface, const int secondSubface) {
  using Setup = HaloProfileSetup;
  std::vector<double> face(Setup::ValuesPerFace, 0.0);
  for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
    for (int j = 0; j < Setup::SecondExtent; j++) {
      for (int i = 0; i < Setup::DofsPerAxis; i++) {
        const double firstPosition  = getFinePosition(i, firstSubface);
        const double secondPosition = DIMENSIONS == 3 ? getFinePosition(j, secondSubface) : 0.0;
        const int volume = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
        for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
          face[volume * Setup::NumberOfData + unknown] =
            getLinearHaloProfile(layer, firstPosition, secondPosition, unknown);
        }
      }
    }
  }
  return face;
}
