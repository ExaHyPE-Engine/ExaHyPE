// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
#include "exahype2/dg/limiting/Admissibility.h"
#include "exahype2/dg/limiting/FaceStatus.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/VertexExtrema.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::limiting {
  template <typename SolverType, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics reduceSubcellMaxEigenvalues(
    const Value*                                 subcellSolution,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
} // namespace exahype2::dg::limiting
TARGET_DECLARE_END

namespace exahype2::dg::limiting {
  template <typename SolverType, bool UsesAdmissibilityCriterion, typename Value, typename CellLabel>
  INLINE_METHOD bool isCandidateAdmissible(
    const peano4::dataview::CellMarker& marker,
    const CellLabel&                    cellLabel,
    const Value*                        candidate,
    const Value*                        candidateSubcells
  );

  template <typename SolverType, typename Value, typename CellLabel, typename FaceLabels>
  INLINE_METHOD void commitLimiterVerdict(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabels&                   faceLabels,
    bool                                admissible,
    const Value*                        candidate,
    const Value*                        candidateSubcells,
    const Value*                        quadraturePoints1D
  );

  template <
    typename SolverType,
    bool UsesAdmissibilityCriterion,
    typename Value,
    typename CellLabel,
    typename FaceLabels>
  INLINE_METHOD void correctAndFlag(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabels&                   faceLabels,
    const Value*                        candidate,
    const Value*                        dgToFvMatrix,
    const Value*                        quadraturePoints1D
  );

  template <
    typename SolverType,
    bool UsesAdmissibilityCriterion,
    int  NumberOfObservables,
    bool UsesRelaxationTerm,
    typename Value,
    typename CellLabel,
    typename FaceLabels,
    typename VertexLabels>
  INLINE_METHOD void correctAndFlag(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabels&                   faceLabels,
    const VertexLabels&                 vertexLabels,
    const Value*                        candidate,
    const Value*                        previousSolution,
    const Value*                        dgToFvMatrix,
    const Value*                        dgToLobattoMatrix,
    const Value*                        dgToSubcellMatrix,
    const Value*                        quadraturePoints1D,
    Value                               relaxationParameter,
    Value                               differencesScaling
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/CorrectAndFlag.cpph"
