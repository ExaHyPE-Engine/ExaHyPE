// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Landslide.h"
#include "applications/exaskred/LandslideLimiting.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"

namespace {
  constexpr applications::exahype2::exaskred::landslide::Physics LandslidePhysics{
    applications::exahype2::exaskred::g,
    applications::exahype2::exaskred::hThreshold,
    applications::exahype2::exaskred::mu,
    applications::exahype2::exaskred::invXi,
    applications::exahype2::exaskred::rhoSlide,
    applications::exahype2::exaskred::rhoWater,
    applications::exahype2::exaskred::waterLevel,
    applications::exahype2::exaskred::waterFrictionDragCoefficient,
    applications::exahype2::exaskred::waterPressureDragCoefficient
  };
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                                         Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  const auto bathymetrySize   = applications::exahype2::exaskred::RuntimeParameters::getBathymetrySize();
  const auto bathymetryOffset = applications::exahype2::exaskred::RuntimeParameters::getBathymetryOffset();
  static tarch::reader::NetCDFFieldSampler<2> bathymetrySampler(
    {{"bathymetry", applications::exahype2::exaskred::RuntimeParameters::getBathymetryFile(), "Band1", {"x", "y"}}},
    {bathymetrySize(0), bathymetrySize(1)},
    {bathymetryOffset(0), bathymetryOffset(1)}
  );

  const auto displacementSize     = applications::exahype2::exaskred::RuntimeParameters::getDisplacementSize();
  const auto displacementOffset   = applications::exahype2::exaskred::RuntimeParameters::getDisplacementOffset();
  const auto displacementPosition = applications::exahype2::exaskred::RuntimeParameters::getDisplacementPosition();
  static tarch::reader::NetCDFFieldSampler<2> displacementSampler(
    {{"displacement", applications::exahype2::exaskred::RuntimeParameters::getDisplacementFile(), "Band1", {"x", "y"}}},
    {displacementSize(0), displacementSize(1)},
    {displacementOffset(0), displacementOffset(1)}
  );

  const double displacement = displacementSampler.sampleBilinear(
    "displacement",
    {x(0) - displacementPosition(0), x(1) - displacementPosition(1)}
  );

  Q[Shortcuts::h] = displacement > 0.0 ? displacement : 0.0;
  Q[Shortcuts::z] = bathymetrySampler.sampleBilinear("bathymetry", {x(0), x(1)});
}

void applications::exahype2::exaskred::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                                   Qinside,
  double* const NOALIAS                                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::exaskred::ADERDGSolver::refinementCriterion(
  [[maybe_unused]] const double* const NOALIAS                  Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t
) {
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
  ::applications::exahype2::exaskred::landslide::flux<Shortcuts, NumberOfUnknowns>(
    Q,
    x,
    h,
    t,
    dt,
    normal,
    LandslidePhysics,
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
  return ::applications::exahype2::exaskred::landslide::effectiveMaxEigenvalue<
    Shortcuts>(Q, x, h, t, dt, normal, LandslidePhysics, PNPM / static_cast<double>(DIMENSIONS));
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
  ::applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
    Q,
    deltaQ,
    x,
    h,
    t,
    dt,
    normal,
    LandslidePhysics,
    BTimesDeltaQ
  );
}

void applications::exahype2::exaskred::ADERDGSolver::diffusiveSourceTerm(
  const double* const NOALIAS                  Q,
  const double* const NOALIAS                  deltaQ,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  double* const NOALIAS                        S
) {
  ::applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
    Shortcuts,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(Q, deltaQ, x, h, t, dt, LandslidePhysics, S);
}

// Both Riemann solvers see sanitised states, so a wet-dry front never feeds
// stale momenta of formally dry nodes into the dissipation or the jump terms.
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
  [[maybe_unused]] const bool                  isBoundaryFace,
  [[maybe_unused]] const int                   faceIndex
) {
  constexpr int NumberOfFaceNodes = Order + 1;
  constexpr int NumberOfData      = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  double        sanitisedQL[NumberOfFaceNodes * NumberOfData];
  double        sanitisedQR[NumberOfFaceNodes * NumberOfData];
  for (int node = 0; node < NumberOfFaceNodes; node++) {
    ::applications::exahype2::exaskred::landslide::
      sanitiseDryState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        QL + node * NumberOfData,
        sanitisedQL + node * NumberOfData,
        LandslidePhysics
      );
    ::applications::exahype2::exaskred::landslide::
      sanitiseDryState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        QR + node * NumberOfData,
        sanitisedQR + node * NumberOfData,
        LandslidePhysics
      );
  }
  const auto maxEigenvalue = [](
                               const double* const NOALIAS                  Q,
                               const tarch::la::Vector<DIMENSIONS, double>& facePosition,
                               const tarch::la::Vector<DIMENSIONS, double>& faceSize,
                               const double                                 time,
                               const double                                 timeStepSize,
                               const int                                    normal
                             ) {
    return ::applications::exahype2::exaskred::landslide::fluxEigenvalue<Shortcuts>(
      Q,
      facePosition,
      faceSize,
      time,
      timeStepSize,
      normal,
      LandslidePhysics
    );
  };
  const auto interfaceGravity = [](const double* const NOALIAS QLeft, const double* const NOALIAS QRight) {
    return ::applications::exahype2::exaskred::landslide::interfaceEffectiveGravity<Shortcuts>(
      QLeft,
      QRight,
      LandslidePhysics
    );
  };
  ::applications::exahype2::exaskred::riemann::
    wellBalancedRusanov<Shortcuts, Shortcuts::z, NumberOfUnknowns, NumberOfAuxiliaryVariables, Order>(
      sanitisedQL,
      sanitisedQR,
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
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  double        sanitisedQL[NumberOfData];
  double        sanitisedQR[NumberOfData];
  ::applications::exahype2::exaskred::landslide::
    sanitiseDryState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(QL, sanitisedQL, LandslidePhysics);
  ::applications::exahype2::exaskred::landslide::
    sanitiseDryState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(QR, sanitisedQR, LandslidePhysics);
  return ::applications::exahype2::exaskred::riemann::augmentedState<Shortcuts, Shortcuts::z, true>(
    sanitisedQL,
    sanitisedQR,
    x,
    h,
    t,
    dt,
    normal,
    ::applications::exahype2::exaskred::landslide::interfaceEffectiveGravity<Shortcuts>(
      sanitisedQL,
      sanitisedQR,
      LandslidePhysics
    ),
    LandslidePhysics.dryHeightThreshold,
    FL,
    FR
  );
}

// The granular layer is checked per state for finiteness, non-negativity, rest
// on dry ground, and an integrable friction stiffness; the nodal candidate
// additionally keeps its height surface below the curvature bound.
bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                                   Q,
  const int                                                     numberOfStates,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>&                  h
) const {
  if (not ::applications::exahype2::exaskred::landslide::
        isAdmissibleGranularLayerCell<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          Q,
          numberOfStates,
          h,
          LandslidePhysics,
          maxFrictionStiffnessRatio
        )) {
    return false;
  }
  const int nodesPerAxis = static_cast<int>(std::lround(std::sqrt(static_cast<double>(numberOfStates))));
  if (numberOfStates > 1 and nodesPerAxis * nodesPerAxis == numberOfStates) {
    return ::applications::exahype2::exaskred::landslide::
      hasAdmissibleGranularLayerHeightCurvature2D<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        Q,
        nodesPerAxis,
        LandslidePhysics,
        maxHeightCurvatureRatio
      );
  }
  return true;
}

void applications::exahype2::exaskred::ADERDGSolver::mapDmpObservables(
  double* const NOALIAS       observables,
  const double* const NOALIAS Q
) const {
  ::applications::exahype2::exaskred::landslide::mapGranularLayerDmpObservables<Shortcuts>(observables, Q);
}

double applications::exahype2::exaskred::ADERDGSolver::dmpRelaxationParameter(
  const int    observable,
  const double specifiedRelaxationParameter
) const {
  return ::applications::exahype2::exaskred::landslide::granularLayerDmpRelaxationParameter(
    observable,
    specifiedRelaxationParameter,
    LandslidePhysics
  );
}
