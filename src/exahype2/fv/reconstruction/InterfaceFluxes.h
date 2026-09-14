// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::fv::reconstruction {
  template <int HaloSize, int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD const StoragePrecision* neighbourAlongNormal(
    const StoragePrecision* NOALIAS const     inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[TWO_TIMES_D],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       centreOffset,
    int                                       strideAlongNormal,
    int                                       normal,
    int                                       offset
  );

  template <typename Solver, int NumberOfUnknowns, bool EvaluateFlux, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD ComputePrecision rusanovFaceFlux(
    const ComputePrecision* NOALIAS const        leftState,
    const ComputePrecision* NOALIAS const        rightState,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    int                                          normal,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS const              faceFlux
  );

  template <
    typename Solver,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfData,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateFluxDivergenceFromFaceStates(
    const ComputePrecision* NOALIAS const        leftNeighbourPlus,
    const ComputePrecision* NOALIAS const        centreMinus,
    const ComputePrecision* NOALIAS const        centrePlus,
    const ComputePrecision* NOALIAS const        rightNeighbourMinus,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    int                                          normal,
    double                                       timeStamp,
    double                                       timeStepSize,
    StoragePrecision* NOALIAS const              outputState,
    double&                                      maxEigenvalueAfterTimeStep
  );
} // namespace exahype2::fv::reconstruction
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/InterfaceFluxes.cpph"
