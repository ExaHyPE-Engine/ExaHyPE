// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/ader/CauchyKovalewski.h"
#include "exahype2/dg/ader/LinearPredictorTerms.h"
#include "exahype2/dg/ader/PicardIteration.h"
#include "exahype2/dg/ader/PicardOutcome.h"
#include "exahype2/dg/ader/PointSourceContribution.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimeTerms.h"
#include "exahype2/dg/ader/TimeAveraging.h"
#include "exahype2/dg/ader/VolumeIntegral.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveFlux,
    bool EvaluateDiffusiveSource,
    typename StoragePrecision,
    typename ComputePrecision,
    typename FacePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD int spaceTimePredictor(
    FacePrecision* const NOALIAS                 faceState[peano4::FacesPerCell],
    FacePrecision* const NOALIAS                 faceFlux[peano4::FacesPerCell],
    StoragePrecision* NOALIAS                    cellState,
    ComputePrecision* NOALIAS                    scratch,
    const BasisTables<ComputePrecision>&         tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double                                       picardTolerance
  );

  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaterialParameters,
    bool EvaluatePointSources,
    typename StoragePrecision,
    typename ComputePrecision,
    typename FacePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void linearSpaceTimePredictor(
    FacePrecision* const NOALIAS                 faceState[peano4::FacesPerCell],
    FacePrecision* const NOALIAS                 faceFlux[peano4::FacesPerCell],
    StoragePrecision* NOALIAS                    cellState,
    ComputePrecision* NOALIAS                    scratch,
    const int* NOALIAS                           pointSourceIndices,
    int                                          numberOfPointSources,
    const double* NOALIAS                        pointSourceLocations,
    const BasisTables<ComputePrecision>&         tables,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/SpaceTimePredictor.cpph"
