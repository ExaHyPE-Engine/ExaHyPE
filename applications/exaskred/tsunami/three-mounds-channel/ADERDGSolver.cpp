// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"

namespace {
  double moundBathymetry(const tarch::la::Vector<DIMENSIONS, double>& position) {
    const auto moundOne =
      1.0 - 0.10 * std::sqrt((position[0] - 15.0) * (position[0] - 15.0) + (position[1] - 22.5) * (position[1] - 22.5));
    const auto moundTwo =
      1.0 - 0.10 * std::sqrt((position[0] - 15.0) * (position[0] - 15.0) + (position[1] - 7.5) * (position[1] - 7.5));
    const auto moundThree =
      1.0 - 0.28 * std::sqrt((position[0] - 28.0) * (position[0] - 28.0) + (position[1] - 15.0) * (position[1] - 15.0));
    return std::max({0.0, moundOne, moundTwo, moundThree});
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  (void)h;

  for (int variable = 0; variable < NumberOfUnknowns + NumberOfAuxiliaryVariables; variable++) {
    Q[variable] = 0.0;
  }

  constexpr double InitialWaterHeightDam = 0.9;
  Q[Shortcuts::h]                        = x[0] <= 5.0 ? InitialWaterHeightDam : 0.0;
  Q[Shortcuts::b]                        = moundBathymetry(x);
}

void applications::exahype2::exaskred::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const int                                    normal
) {
  (void)x;
  (void)h;
  (void)t;
  (void)normal;

  Qoutside[Shortcuts::h]      = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu]     = 0.0;
  Qoutside[Shortcuts::hu + 1] = 0.0;
  Qoutside[Shortcuts::b]      = Qinside[Shortcuts::b];
}

::exahype2::RefinementCommand applications::exahype2::exaskred::ADERDGSolver::refinementCriterion(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t
) {
  (void)Q;
  (void)h;
  (void)t;

  return moundBathymetry(x) > 0.0 ? ::exahype2::RefinementCommand::Refine : ::exahype2::RefinementCommand::Keep;
}

void applications::exahype2::exaskred::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  ::applications::exahype2::exaskred::shallowwater::flux<Shortcuts, NumberOfUnknowns>(
    Q,
    x,
    h,
    t,
    dt,
    normal,
    hThreshold,
    F
  );
}

double applications::exahype2::exaskred::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal
) {
  return ::applications::exahype2::exaskred::shallowwater::maxEigenvalue<
    Shortcuts>(Q, x, h, t, dt, normal, g, hThreshold);
}

void applications::exahype2::exaskred::ADERDGSolver::nonconservativeProduct(
  const double* const NOALIAS                  Q,
  const double* const NOALIAS                  deltaQ,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  double* const NOALIAS                        BTimesDeltaQ
) {
  ::applications::exahype2::exaskred::shallowwater::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
    Q,
    deltaQ,
    x,
    h,
    t,
    dt,
    normal,
    g,
    hThreshold,
    BTimesDeltaQ
  );
}

double applications::exahype2::exaskred::ADERDGSolver::solveRiemannProblem(
  const double* const NOALIAS                  QL,
  const double* const NOALIAS                  QR,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  double* const NOALIAS                        FL,
  double* const NOALIAS                        FR
) {
  return ::applications::exahype2::exaskred::riemann::augmentedState<Shortcuts, Shortcuts::b, true>(
    QL,
    QR,
    x,
    h,
    t,
    dt,
    normal,
    g,
    hThreshold,
    FL,
    FR
  );
}

bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                  Q,
  const int                                    numberOfStates,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) const {
  (void)x;
  (void)h;

  if (numberOfStates == 1) {
    return ::applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, 0.0);
  }
  return ::applications::exahype2::exaskred::limiting::
    hasBoundedDepthVariation<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
      Q,
      numberOfStates,
      std::max(100.0 * hThreshold, 0.045),
      0.35
    );
}

void applications::exahype2::exaskred::ADERDGSolver::riemannSolver(
  double* const NOALIAS                        FL,
  double* const NOALIAS                        FR,
  const double* const NOALIAS                  QL,
  const double* const NOALIAS                  QR,
  const double                                 t,
  const double                                 dt,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const int                                    direction,
  const bool                                   isBoundaryFace,
  const int                                    faceIndex
) {
  (void)isBoundaryFace;
  (void)faceIndex;

  const auto maxEigenvalue = [](
                               const double* const NOALIAS                  Q,
                               const tarch::la::Vector<DIMENSIONS, double>& facePosition,
                               const tarch::la::Vector<DIMENSIONS, double>& faceSize,
                               const double                                 time,
                               const double                                 timeStepSize,
                               const int                                    normal
                             ) {
    return ::applications::exahype2::exaskred::shallowwater::maxEigenvalue<
      Shortcuts>(Q, facePosition, faceSize, time, timeStepSize, normal, g, hThreshold);
  };
  const auto interfaceGravity =
    []([[maybe_unused]] const double* const NOALIAS QLeft, [[maybe_unused]] const double* const NOALIAS QRight) {
      return g;
    };
  ::applications::exahype2::exaskred::riemann::
    wellBalancedRusanov<Shortcuts, Shortcuts::b, NumberOfUnknowns, NumberOfAuxiliaryVariables, DGOrder>(
      QL,
      QR,
      x,
      h,
      t,
      dt,
      direction,
      maxEigenvalue,
      interfaceGravity,
      FL,
      FR
    );
}
