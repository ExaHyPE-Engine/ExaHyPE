// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
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
    bool IsLinear,
    bool UseRiemannSolverTerm,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateMaterialParameters,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveRiemannProblemOnEstimates(
    StoragePrecision* NOALIAS                    leftFluxEstimates,
    StoragePrecision* NOALIAS                    rightFluxEstimates,
    const StoragePrecision* NOALIAS              leftStateEstimates,
    const StoragePrecision* NOALIAS              rightStateEstimates,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    bool                                         isBoundary,
    int                                          faceNumber
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

namespace exahype2::dg::ader {
  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool IsLinear,
    bool HangingFace,
    bool UseRiemannSolverTerm,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateMaterialParameters,
    typename StoragePrecision,
    typename ComputePrecision,
    typename FaceLabel,
    typename FaceLabelEnumerator>
  INLINE_METHOD void solveRiemannProblemOnFace(
    const peano4::dataview::FaceMarker& marker,
    FaceLabel&                          faceLabel,
    const FaceLabelEnumerator&          coarseFaceLabels,
    StoragePrecision* NOALIAS           faceState,
    StoragePrecision* NOALIAS           faceFlux,
    const ComputePrecision* NOALIAS     quadraturePoints,
    const ComputePrecision* NOALIAS     faceQuadratureWeights,
    double                              timeStepSize
  );

  template <typename FaceLabel, typename FaceLabelEnumerator>
  INLINE_METHOD void mergeHangingFaceTimeStamps(
    const peano4::dataview::FaceMarker& marker,
    FaceLabel&                          faceLabel,
    const FaceLabelEnumerator&          coarseFaceLabels
  );

  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool IsLinear,
    bool ReportEigenvalues,
    typename StoragePrecision,
    typename ComputePrecision,
    typename CellLabel,
    typename FaceLabelEnumerator,
    typename FaceFluxEnumerator,
    typename Postprocessing>
  INLINE_METHOD void correct(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabelEnumerator&          faceLabels,
    const FaceFluxEnumerator&           faceFluxes,
    StoragePrecision* NOALIAS           cellState,
    const ComputePrecision* NOALIAS     faceQuadratureWeights,
    const ComputePrecision* NOALIAS     inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS     leftFaceCoefficients,
    const ComputePrecision* NOALIAS     rightFaceCoefficients,
    const StoragePrecision* NOALIAS     stateQuadraturePoints,
    Postprocessing&&                    postprocessCell
  );
} // namespace exahype2::dg::ader

#include "exahype2/dg/ader/Correct.cpph"
