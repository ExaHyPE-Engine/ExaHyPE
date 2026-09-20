// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimitingSolverState.h"

#include "tarch/Assertions.h"

/**
 * Advances the a-posteriori limiting pipeline by one grid sweep.
 *
 * A time step predicts, then corrects and flags, and ends there when no cell is troubled, so a
 * smooth run costs two sweeps. A troubled cell grows the finite-volume domain ring by ring, one
 * sweep and one face-status exchange per ring, before the troubled cells are recomputed on their
 * subgrid; the ring count is a property of the subcell stencil and not of the decomposition, so
 * every partition recomputes the same domain.
 *
 * @p decision carries the two branch flags, both reduced across ranks before
 * the transition so every rank walks the same path.
 */
exahype2::dg::LimitingSweep exahype2::dg::advanceLimitingSweep(
  const exahype2::dg::LimitingSweep          current,
  const exahype2::dg::LimitingBranchDecision decision,
  const int                                  numberOfSpreadRings
) {
  assertion(numberOfSpreadRings >= 1);
  switch (current.state) {
  case exahype2::dg::LimitingSolverState::GridConstruction:
    return {exahype2::dg::LimitingSolverState::GridInitialisation, 0};
  case exahype2::dg::LimitingSolverState::GridInitialisation:
  case exahype2::dg::LimitingSolverState::Plotting:
  case exahype2::dg::LimitingSolverState::Suspended:
  case exahype2::dg::LimitingSolverState::RecomputeSubcells:
    return {exahype2::dg::LimitingSolverState::Predict, 0};
  case exahype2::dg::LimitingSolverState::Predict:
    return {
      decision.hasHangingFaces ? exahype2::dg::LimitingSolverState::PredictOnHangingCells
                               : exahype2::dg::LimitingSolverState::CorrectAndFlag,
      0
    };
  case exahype2::dg::LimitingSolverState::PredictOnHangingCells:
    return {exahype2::dg::LimitingSolverState::CorrectAndFlag, 0};
  case exahype2::dg::LimitingSolverState::CorrectAndFlag:
    return decision.hasTroubledCells ? exahype2::dg::LimitingSweep{exahype2::dg::LimitingSolverState::SpreadDomain, 0}
                                     : exahype2::dg::LimitingSweep{exahype2::dg::LimitingSolverState::Predict, 0};
  case exahype2::dg::LimitingSolverState::SpreadDomain:
    return current.spreadRing + 1 < numberOfSpreadRings
      ? exahype2::dg::LimitingSweep{exahype2::dg::LimitingSolverState::SpreadDomain, current.spreadRing + 1}
      : exahype2::dg::LimitingSweep{exahype2::dg::LimitingSolverState::RecomputeSubcells, 0};
  }
  return current;
}

/**
 * Returns whether a sweep opens a time step. The predictor is the sweep that
 * resets the per-step accumulators.
 */
bool exahype2::dg::isFirstSweepOfTimeStep(const exahype2::dg::LimitingSweep sweep) {
  return sweep.state == exahype2::dg::LimitingSolverState::Predict;
}

/**
 * Returns whether a sweep closes a time step. A step closes either at the
 * corrector or after the subcell recompute, and the corrector closes it only
 * when the reduced decision reports no troubled cell.
 */
bool exahype2::dg::isLastSweepOfTimeStep(
  const exahype2::dg::LimitingSweep          sweep,
  const exahype2::dg::LimitingBranchDecision decision
) {
  const bool correctorAccepted =
    sweep.state == exahype2::dg::LimitingSolverState::CorrectAndFlag and not decision.hasTroubledCells;
  const bool subcellsRecomputed = sweep.state == exahype2::dg::LimitingSolverState::RecomputeSubcells;
  return correctorAccepted or subcellsRecomputed;
}

std::string exahype2::dg::toString(const exahype2::dg::LimitingSolverState state) {
  switch (state) {
  case exahype2::dg::LimitingSolverState::GridConstruction:
    return "GridConstruction";
  case exahype2::dg::LimitingSolverState::GridInitialisation:
    return "GridInitialisation";
  case exahype2::dg::LimitingSolverState::Predict:
    return "Predict";
  case exahype2::dg::LimitingSolverState::PredictOnHangingCells:
    return "PredictOnHangingCells";
  case exahype2::dg::LimitingSolverState::CorrectAndFlag:
    return "CorrectAndFlag";
  case exahype2::dg::LimitingSolverState::SpreadDomain:
    return "SpreadDomain";
  case exahype2::dg::LimitingSolverState::RecomputeSubcells:
    return "RecomputeSubcells";
  case exahype2::dg::LimitingSolverState::Plotting:
    return "Plotting";
  case exahype2::dg::LimitingSolverState::Suspended:
    return "Suspended";
  }
  return "<undef>";
}
