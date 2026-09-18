// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Gradient.h"
#include "exahype2/dg/OperatorAlongAxis.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::rk {
  template <
    typename Solver,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool IsLinear,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    bool EvaluateSource,
    bool EvaluateNonconservativeProduct,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void volumeIntegral(
    const StoragePrecision* NOALIAS              cellState,
    StoragePrecision* NOALIAS                    cellUpdate,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              massMatrixDiagonal,
    const ComputePrecision* NOALIAS              stiffnessMatrix,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
} // namespace exahype2::dg::rk
TARGET_DECLARE_END

#include "exahype2/dg/rk/VolumeIntegral.cpph"
