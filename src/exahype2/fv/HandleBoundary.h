// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "peano4/Globals.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::fv {
  template <
    typename Solver,
    int NumberOfVolumesPerAxis,
    int HaloSize,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void handleBoundary(
    Solver&                                      solver,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    const double                                 timeStamp,
    const int                                    faceNumber,
    StoragePrecision* const                      faceData
  );
} // namespace exahype2::fv
TARGET_DECLARE_END

#include "HandleBoundary.cpph"
