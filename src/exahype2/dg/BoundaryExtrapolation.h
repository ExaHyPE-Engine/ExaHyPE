// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Gradient.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateStateToFaces(
    StoragePrecision* const NOALIAS faceState[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS cellState,
    const ComputePrecision* NOALIAS leftFaceCoefficients,
    const ComputePrecision* NOALIAS rightFaceCoefficients
  );

  template <
    typename Solver,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateFluxToFaces(
    StoragePrecision* const NOALIAS              faceFlux[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              cellState,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <
    typename Solver,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateFluxOnFace(
    StoragePrecision* NOALIAS                    faceFlux,
    const StoragePrecision* NOALIAS              leftState,
    const StoragePrecision* NOALIAS              rightState,
    int                                          side,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/BoundaryExtrapolation.cpph"
