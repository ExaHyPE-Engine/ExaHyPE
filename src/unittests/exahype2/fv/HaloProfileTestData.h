// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2::fv::unittests {
  struct HaloProfileSetup;

  double getLinearHaloProfile(int layer, double firstPosition, double secondPosition, int unknown);
  double getFinePosition(int volume, int subface);
  std::vector<double> getFineFaceUnknownsWithProfile(int normal, int firstSubface, int secondSubface);
  std::vector<double> getFineFaceAuxVarsWithProfile(int normal, int firstSubface, int secondSubface);
} // namespace exahype2::fv::unittests

TARGET_DECLARE
/**
 * The patch and halo extents of the transfer tests.
 */
struct exahype2::fv::unittests::HaloProfileSetup final {
  static constexpr int DofsPerAxis                = 3;
  static constexpr int HaloSize                   = 2;
  static constexpr int NumberOfUnknowns           = 1;
  static constexpr int NumberOfAuxiliaryVariables = 1;
  static constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int SubdivisionFactor          = peano4::SubdivisionFactor;
  static constexpr int ChildrenPerCell            = peano4::ChildrenPerCell;
  static constexpr int VolumesPerCell             = peano4::power(DofsPerAxis, DIMENSIONS);
  static constexpr int VolumesPerLayer            = peano4::power(DofsPerAxis, DIMENSIONS - 1);
  static constexpr int VolumesPerFace             = 2 * HaloSize * VolumesPerLayer;
  static constexpr int ValuesPerCell              = VolumesPerCell * NumberOfData;
  static constexpr int UnknownsPerFace            = VolumesPerFace * NumberOfUnknowns;
  static constexpr int AuxVarsPerFace             = VolumesPerFace * NumberOfAuxiliaryVariables;
  static constexpr int SecondExtent               = DIMENSIONS == 3 ? DofsPerAxis : 1;
};
TARGET_DECLARE_END
