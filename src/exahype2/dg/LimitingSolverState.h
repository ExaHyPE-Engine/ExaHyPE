// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::dg {
  enum class LimitingSolverState : int;
  struct LimitingBranchDecision;
  struct LimitingSweep;
} // namespace exahype2::dg

enum class exahype2::dg::LimitingSolverState : int {
  GridConstruction,
  GridInitialisation,
  Predict,
  PredictOnHangingCells,
  CorrectAndFlag,
  SpreadDomain,
  RecomputeSubcells,
  Plotting,
  Suspended
};

struct exahype2::dg::LimitingBranchDecision final {
  bool hasHangingFaces{false};
  bool hasTroubledCells{false};
};

struct exahype2::dg::LimitingSweep final {
  exahype2::dg::LimitingSolverState state{exahype2::dg::LimitingSolverState::GridConstruction};
  int                               spreadRing{0};
};

namespace exahype2::dg {
  LimitingSweep advanceLimitingSweep(
    const LimitingSweep          current,
    const LimitingBranchDecision decision,
    const int                    numberOfSpreadRings
  );

  bool isFirstSweepOfTimeStep(const LimitingSweep sweep);

  bool isLastSweepOfTimeStep(const LimitingSweep sweep, const LimitingBranchDecision decision);

  std::string toString(const LimitingSolverState state);
} // namespace exahype2::dg
