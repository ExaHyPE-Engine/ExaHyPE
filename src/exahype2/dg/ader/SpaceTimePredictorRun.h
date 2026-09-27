// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/PointSources.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimePredictor.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::dg::ader {
  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool IsLinear,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveFlux,
    bool EvaluateDiffusiveSource,
    bool EvaluateMaterialParameters,
    bool EvaluatePointSources,
    typename ComputePrecision,
    typename StoragePrecision,
    typename FacePrecision>
  INLINE_METHOD int runSpaceTimePredictor(
    const peano4::dataview::CellMarker&  marker,
    FacePrecision* const*                stateEstimates,
    FacePrecision* const*                fluxEstimates,
    StoragePrecision* NOALIAS            cellState,
    const BasisTables<ComputePrecision>& tables,
    double                               timeStamp,
    double                               timeStepSize
  );
} // namespace exahype2::dg::ader

#include "exahype2/dg/ader/SpaceTimePredictorRun.cpph"
