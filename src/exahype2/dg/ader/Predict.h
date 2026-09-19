// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimePredictorRun.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/la/VectorOperations.h"

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
    typename PicardPrecision,
    typename StoragePrecision,
    typename CellLabel,
    typename FaceLabelEnumerator,
    typename FaceStateEnumerator,
    typename FaceFluxEnumerator>
  INLINE_METHOD void predict(
    const peano4::dataview::CellMarker&  marker,
    SolverType&                          solver,
    CellLabel&                           cellLabel,
    const FaceLabelEnumerator&           faceLabels,
    const FaceStateEnumerator&           faceStates,
    const FaceFluxEnumerator&            faceFluxes,
    StoragePrecision* NOALIAS            cellState,
    StoragePrecision* NOALIAS            previousCellState,
    const BasisTables<ComputePrecision>& tables,
    const BasisTables<PicardPrecision>&  picardTables,
    double                               timeStepSize
  );
} // namespace exahype2::dg::ader

#include "exahype2/dg/ader/Predict.cpph"
