// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/OperatorAlongAxis.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <int NumberOfDofsPerAxis, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void buildVolumeCoefficients(
    ComputePrecision* NOALIAS       volumeCoefficients,
    const ComputePrecision* NOALIAS stiffnessOperator,
    const ComputePrecision* NOALIAS inverseQuadratureWeights,
    double                          timeStepSize
  );

  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addFluxVolumeIntegral(
    StoragePrecision* NOALIAS                              cellState,
    const ComputePrecision* NOALIAS                        timeAveragedFlux,
    const ComputePrecision* NOALIAS                        volumeCoefficients,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize
  );

  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addSourceVolumeIntegral(
    StoragePrecision* NOALIAS       cellState,
    const ComputePrecision* NOALIAS timeAveragedSource,
    double                          timeStepSize
  );

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool AccumulatesFlux,
    bool EvaluateSource,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void subtractTimeAveragedVolumeIntegral(
    StoragePrecision* NOALIAS       cellState,
    const ComputePrecision* NOALIAS timeAveragedFlux,
    const ComputePrecision* NOALIAS timeAveragedSource,
    double                          timeStepSize
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/VolumeIntegral.cpph"
