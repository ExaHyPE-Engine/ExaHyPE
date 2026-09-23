// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"

namespace {
  constexpr double LakeRadius        = 1.0;
  constexpr double ReferenceDepth    = 0.1;
  constexpr double OrbitDisplacement = 0.5;

  double lakeBathymetry(const tarch::la::Vector<DIMENSIONS, double>& position) {
    const auto radiusSquared = position[0] * position[0] + position[1] * position[1];
    return -ReferenceDepth * (1.0 - radiusSquared / (LakeRadius * LakeRadius));
  }

  template <class Shortcuts, int NumberOfData>
  void setOscillatingLakeState(
    double* const                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 timeStamp,
    const double                                 bathymetry
  ) {
    for (int variable = 0; variable < NumberOfData; variable++) {
      Q[variable] = 0.0;
    }

    const auto angularFrequency = std::sqrt(2.0 * applications::exahype2::exaskred::g * ReferenceDepth) / LakeRadius;
    const auto exactHeight      = ((OrbitDisplacement * ReferenceDepth) / (LakeRadius * LakeRadius))
        * (2.0 * position[0] * std::cos(angularFrequency * timeStamp)
           + 2.0 * position[1] * std::sin(angularFrequency * timeStamp) - OrbitDisplacement)
      - bathymetry;

    Q[Shortcuts::b] = bathymetry;
    if (exactHeight > 0.0) {
      Q[Shortcuts::h]  = exactHeight;
      Q[Shortcuts::hu] = -OrbitDisplacement * angularFrequency * std::sin(angularFrequency * timeStamp) * exactHeight;
      Q[Shortcuts::hu + 1] =
        OrbitDisplacement * angularFrequency * std::cos(angularFrequency * timeStamp) * exactHeight;
    }
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  (void)h;

  setOscillatingLakeState<Shortcuts, NumberOfUnknowns + NumberOfAuxiliaryVariables>(Q, x, 0.0, lakeBathymetry(x));
}

void applications::exahype2::exaskred::ADERDGSolver::analyticalSolution(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const double                                 t,
  const double                                 dt,
  const double* const NOALIAS                  Q,
  double* const NOALIAS                        solution
) {
  (void)dt;

  setOscillatingLakeState<Shortcuts, NumberOfUnknowns>(solution, x, t, Q[Shortcuts::b]);
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
  Qoutside[Shortcuts::hu]     = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hu + 1] = -Qinside[Shortcuts::hu + 1];
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

  const auto radiusSquared = x[0] * x[0] + x[1] * x[1];
  return radiusSquared <= LakeRadius * LakeRadius ? ::exahype2::RefinementCommand::Refine
                                                  : ::exahype2::RefinementCommand::Keep;
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
) {
  (void)x;
  (void)h;

  if (numberOfStates == 1) {
    return ::applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, 0.0);
  }
  const auto statistics = ::applications::exahype2::exaskred::limiting::
    collectCellStatistics<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, numberOfStates);
  const auto minimumDgWaterHeight = 100.0 * hThreshold;
  const auto fullyWet             = statistics.minHeight > minimumDgWaterHeight;
  const auto dryAtRest = statistics.minHeight >= 0.0 and statistics.maxHeight <= 0.0 and statistics.maxMomentum <= 0.0;
  return statistics.finite and (fullyWet or dryAtRest);
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
