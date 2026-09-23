// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/Coupled.h"
#include "applications/exaskred/LandslideLimiting.h"

namespace {
  constexpr applications::exahype2::exaskred::landslide::Physics SlidePhysics{
    applications::exahype2::exaskred::g,
    applications::exahype2::exaskred::hThresholdSlide,
    applications::exahype2::exaskred::mu,
    applications::exahype2::exaskred::invXi,
    applications::exahype2::exaskred::rhoSlide,
    applications::exahype2::exaskred::rhoWater,
    0.0,
    applications::exahype2::exaskred::waterFrictionDragCoefficient,
    applications::exahype2::exaskred::waterPressureDragCoefficient
  };

  // The per-state admissibility check is layer-generic, so the water layer
  // reuses it with its own dry threshold and no friction model.
  constexpr applications::exahype2::exaskred::landslide::Physics WaterAdmissibilityPhysics{
    applications::exahype2::exaskred::g,
    applications::exahype2::exaskred::hThresholdWater,
    0.0,
    0.0,
    applications::exahype2::exaskred::rhoWater,
    applications::exahype2::exaskred::rhoWater,
    0.0,
    0.0,
    0.0
  };

  double reservoirTerrain(const tarch::la::Vector<DIMENSIONS, double>& position) {
    return std::max(20.0 - position[0], 1.0);
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                                         Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double SlideCentreX     = 4.0;
  constexpr double SlideCentreY     = 30.0;
  constexpr double SlideRadius      = 3.5;
  constexpr double SlideHeight      = 8.0;
  constexpr double ReservoirSurface = 5.0;

  const tarch::la::Vector<DIMENSIONS, double> slideCentre = {SlideCentreX, SlideCentreY};
  Q[Shortcuts::z]                                         = reservoirTerrain(x);
  Q[Shortcuts::lsh] = tarch::la::norm2(x - slideCentre) < SlideRadius ? SlideHeight : 0.0;
  Q[Shortcuts::h]   = std::max(ReservoirSurface - Q[Shortcuts::z] - Q[Shortcuts::lsh], 0.0);
}

void applications::exahype2::exaskred::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                                   Qinside,
  double* const NOALIAS                                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  Qoutside[Shortcuts::h]        = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu]       = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hu + 1]   = -Qinside[Shortcuts::hu + 1];
  Qoutside[Shortcuts::lsh]      = 0.0;
  Qoutside[Shortcuts::lshu]     = 0.0;
  Qoutside[Shortcuts::lshu + 1] = 0.0;
  Qoutside[Shortcuts::z]        = Qinside[Shortcuts::z];
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
  ::applications::exahype2::exaskred::coupled::flux<Shortcuts, NumberOfUnknowns>(
    Q,
    x,
    h,
    t,
    dt,
    normal,
    SlidePhysics,
    hThresholdWater,
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
  return ::applications::exahype2::exaskred::coupled::maxEigenvalue<
    Shortcuts>(Q, x, h, t, dt, normal, SlidePhysics, hThresholdWater, PNPM / static_cast<double>(DIMENSIONS));
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
  ::applications::exahype2::exaskred::coupled::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
    Q,
    deltaQ,
    x,
    h,
    t,
    dt,
    normal,
    SlidePhysics,
    hThresholdWater,
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
  ::applications::exahype2::exaskred::coupled::diffusiveSourceTerm<
    Shortcuts,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(Q, deltaQ, x, h, t, dt, SlidePhysics, hThresholdWater, S);
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
  [[maybe_unused]] const bool                  isBoundaryFace,
  [[maybe_unused]] const int                   faceIndex
) {
  ::applications::exahype2::exaskred::coupled::
    riemannSolver<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, DGOrder>(
      QL,
      QR,
      x,
      h,
      t,
      dt,
      direction,
      SlidePhysics,
      hThresholdWater,
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
  return ::applications::exahype2::exaskred::coupled::solveRiemannProblem<
    Shortcuts,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables>(QL, QR, x, h, t, dt, normal, SlidePhysics, hThresholdWater, FL, FR);
}

// Both layers must be finite, non-negative, and at rest where dry; the
// granular layer additionally keeps its friction stiffness integrable and its
// nodal height surface below the curvature bound.
bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                                   Q,
  const int                                                     numberOfStates,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>&                  h
) {
  using SlideLayer = ::applications::exahype2::exaskred::coupled::SlideLayer<Shortcuts>;
  if (not ::applications::exahype2::exaskred::landslide::
        isAdmissibleGranularLayerCell<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          Q,
          numberOfStates,
          h,
          WaterAdmissibilityPhysics,
          0.0
        )) {
    return false;
  }
  if (not ::applications::exahype2::exaskred::landslide::
        isAdmissibleGranularLayerCell<SlideLayer, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          Q,
          numberOfStates,
          h,
          SlidePhysics,
          maxFrictionStiffnessRatio
        )) {
    return false;
  }
  const int nodesPerAxis = static_cast<int>(std::lround(std::sqrt(static_cast<double>(numberOfStates))));
  if (numberOfStates > 1 and nodesPerAxis * nodesPerAxis == numberOfStates) {
    return ::applications::exahype2::exaskred::landslide::
      hasAdmissibleGranularLayerHeightCurvature2D<SlideLayer, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        Q,
        nodesPerAxis,
        SlidePhysics,
        maxHeightCurvatureRatio
      );
  }
  return true;
}

void applications::exahype2::exaskred::ADERDGSolver::mapDmpObservables(
  double* const NOALIAS       observables,
  const double* const NOALIAS Q
) {
  for (int observable = 0; observable < NumberOfDMPObservables; observable++) {
    observables[observable] = Q[observable];
  }
}

// The water observables relax against a centimetre-scale floor: The dry
// threshold that seeds the layer's admissibility floor sits at the millimetre
// scale, which would flag every wave in a metres-deep basin.
double applications::exahype2::exaskred::ADERDGSolver::dmpRelaxationParameter(
  const int    observable,
  const double specifiedRelaxationParameter
) {
  if (observable < 3) {
    return std::max(specifiedRelaxationParameter, waterDmpRelaxation);
  }
  return ::applications::exahype2::exaskred::landslide::granularLayerDmpRelaxationParameter(
    observable - 3,
    specifiedRelaxationParameter,
    SlidePhysics
  );
}
