// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  (void)h;

  const auto bathymetrySize   = applications::exahype2::exaskred::RuntimeParameters::getBathymetrySize();
  const auto bathymetryOffset = applications::exahype2::exaskred::RuntimeParameters::getBathymetryOffset();
  static tarch::reader::NetCDFFieldSampler<2> bathymetrySampler(
    {{"bathymetry", applications::exahype2::exaskred::RuntimeParameters::getBathymetryFile()}},
    {bathymetrySize(0), bathymetrySize(1)},
    {bathymetryOffset(0), bathymetryOffset(1)}
  );

  const auto displacementSize   = applications::exahype2::exaskred::RuntimeParameters::getDisplacementSize();
  const auto displacementOffset = applications::exahype2::exaskred::RuntimeParameters::getDisplacementOffset();
  static tarch::reader::NetCDFFieldSampler<2> displacementSampler(
    {{"displacement", applications::exahype2::exaskred::RuntimeParameters::getDisplacementFile()}},
    {displacementSize(0), displacementSize(1)},
    {displacementOffset(0), displacementOffset(1)}
  );

  const auto displacementPosition = applications::exahype2::exaskred::RuntimeParameters::getDisplacementPosition();
  const auto bathymetryBeforeEarthquake = bathymetrySampler.sampleBilinear("bathymetry", {x(0), x(1)});
  const auto displacement               = displacementSampler.sampleBilinear(
    "displacement",
    {x(0) - displacementPosition(0), x(1) - displacementPosition(1)}
  );
  const auto bathymetryAfterEarthquake = bathymetryBeforeEarthquake + displacement;

  Q[Shortcuts::h]      = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[Shortcuts::hu]     = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = bathymetryAfterEarthquake;
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
  (void)x;
  (void)h;
  (void)t;

  return ::exahype2::RefinementCommand::Keep;
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
    isWetCell<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, numberOfStates, hThreshold);
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
    wellBalancedRusanov<Shortcuts, Shortcuts::b, NumberOfUnknowns, NumberOfAuxiliaryVariables, Order>(
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
