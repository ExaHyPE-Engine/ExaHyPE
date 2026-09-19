// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::fv {
  template <
    typename SolverType,
    int NumberOfVolumesPerAxis,
    int HaloSize,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void applyBoundaryConditionsOnFace(
    SolverType&                                  solver,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    int                                          faceNumber,
    StoragePrecision*                            faceData
  );
} // namespace exahype2::fv
TARGET_DECLARE_END

#include "exahype2/fv/ApplyBoundaryConditionsOnFace.cpph"
