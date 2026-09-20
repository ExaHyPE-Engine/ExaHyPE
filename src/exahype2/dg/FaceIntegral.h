// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    bool IsLinear,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addFaceIntegral(
    StoragePrecision* NOALIAS       cellUpdate,
    const ComputePrecision* NOALIAS faceFlux,
    const ComputePrecision* NOALIAS faceQuadratureWeights,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS leftFaceCoefficients,
    const ComputePrecision* NOALIAS rightFaceCoefficients,
    int                             direction,
    int                             orientation,
    double                          inverseCellSizeAlongNormal,
    double                          timeStepSize
  );

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfValuesPerNode,
    bool IsLinear,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addFaceIntegrals(
    StoragePrecision* NOALIAS                    cellValues,
    const ComputePrecision* const                faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const ComputePrecision* NOALIAS              inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStepSize
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/FaceIntegral.cpph"
