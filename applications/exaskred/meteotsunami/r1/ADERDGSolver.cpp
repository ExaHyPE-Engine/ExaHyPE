// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Forcing.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"

namespace {
  // The bathymetry raster covers the channel only, while the mesh spans the square
  // that encloses it. Sampling the nearest raster coordinate continues the depth
  // outwards, so the basin keeps its depth up to the mesh boundary.
  tarch::la::Vector<DIMENSIONS, double> clampToRaster(const tarch::la::Vector<DIMENSIONS, double>& x) {
    const auto offset = applications::exahype2::exaskred::RuntimeParameters::getBathymetryOffset();
    const auto size   = applications::exahype2::exaskred::RuntimeParameters::getBathymetrySize();
    tarch::la::Vector<DIMENSIONS, double> clamped;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      clamped(dimension) = std::clamp(x(dimension), offset(dimension), offset(dimension) + size(dimension));
    }
    return clamped;
  }

  /**
   * Gradient of the travelling pressure disturbance that drives the wave.
   *
   * The disturbance is a sine of one period, damped in two steps, moving along the
   * first axis at a fixed speed. The momentum equation reads the field through this
   * gradient alone, so the pressure itself is never formed, and the field does not
   * vary across the channel. The step envelope is constant between its two levels
   * and contributes nothing to the derivative away from the point where it jumps.
   */
  tarch::la::Vector<DIMENSIONS, double> atmosphericPressureGradient(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const double                                 t
  ) {
    const double amplitude = applications::exahype2::exaskred::RuntimeParameters::getPressureAmplitude();
    const double speed     = applications::exahype2::exaskred::RuntimeParameters::getPressureSpeed();
    const double period    = applications::exahype2::exaskred::RuntimeParameters::getPressurePeriod();

    tarch::la::Vector<DIMENSIONS, double> gradient(0.0);

    const double phase    = (x(0) - 0.75 * speed * period) / speed - t;
    const double envelope = std::round(2.0 * phase / period);
    if (envelope < 0.0 or envelope > 1.0) {
      return gradient;
    }

    const double angularFrequency = 2.0 * std::numbers::pi_v<double> / period;
    gradient(0) =
      -amplitude * std::exp(-5.0 * envelope) * std::cos(angularFrequency * phase) * angularFrequency / speed;
    return gradient;
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                                         Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  static tarch::reader::NetCDFFieldSampler<2> bathymetrySampler(
    {{"bathymetry", RuntimeParameters::getBathymetryFile()}}
  );

  const double bathymetry = bathymetrySampler.sampleBilinear("bathymetry", clampToRaster(x));

  Q[Shortcuts::h]      = -std::min(bathymetry, 0.0);
  Q[Shortcuts::hu]     = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = bathymetry;
}

void applications::exahype2::exaskred::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                                   Qinside,
  double* const NOALIAS                                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  const int                                                     normal
) {
  for (int unknown = 0; unknown < NumberOfUnknowns + NumberOfAuxiliaryVariables; unknown++) {
    Qoutside[unknown] = Qinside[unknown];
  }
  // The channel is closed across its width and open along the direction the
  // disturbance travels, so only the transverse momentum reflects.
  if (normal != 0) {
    Qoutside[Shortcuts::hu + 1] = -Qinside[Shortcuts::hu + 1];
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
  ::applications::exahype2::exaskred::shallowwater::flux<Shortcuts, NumberOfUnknowns>(
    Q,
    x,
    h,
    t,
    dt,
    normal,
    RuntimeParameters::getMinimumWaterHeight(),
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
    Shortcuts>(Q, x, h, t, dt, normal, g, RuntimeParameters::getMinimumWaterHeight());
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
    RuntimeParameters::getMinimumWaterHeight(),
    BTimesDeltaQ
  );
}

/**
 * Rotation, bed drag and the atmospheric pressure gradient.
 *
 * The pressure gradient is what makes this a meteotsunami: A disturbance travelling
 * near the shallow-water wave speed keeps feeding the wave it rides on.
 */
void applications::exahype2::exaskred::ADERDGSolver::sourceTerm(
  const double* const NOALIAS                                   Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                                  t,
  [[maybe_unused]] const double                                 dt,
  double* const NOALIAS                                         S
) {
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    S[unknown] = 0.0;
  }
  const double minimumWaterHeight = RuntimeParameters::getMinimumWaterHeight();
  if (not std::isfinite(Q[Shortcuts::h]) or Q[Shortcuts::h] <= minimumWaterHeight) {
    return;
  }

  const double coriolis = RuntimeParameters::getCoriolisParameter();
  const double friction = ::applications::exahype2::exaskred::forcing::manningFriction(
    Q[Shortcuts::h],
    Q[Shortcuts::hu],
    Q[Shortcuts::hu + 1],
    g,
    RuntimeParameters::getManningCoefficient(),
    minimumWaterHeight
  );
  const auto   pressureGradient = atmosphericPressureGradient(x, t);
  const double waterDensity     = RuntimeParameters::getWaterDensity();

  S[Shortcuts::hu] = coriolis * Q[Shortcuts::hu + 1] + friction * Q[Shortcuts::hu]
    - Q[Shortcuts::h] * pressureGradient(0) / waterDensity;
  S[Shortcuts::hu + 1] = -coriolis * Q[Shortcuts::hu] + friction * Q[Shortcuts::hu + 1]
    - Q[Shortcuts::h] * pressureGradient(1) / waterDensity;
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
    RuntimeParameters::getMinimumWaterHeight(),
    FL,
    FR
  );
}

/**
 * A cell is troubled where the water is shallow enough that the polynomial stops
 * describing a body of water.
 *
 * The channel carries a long smooth wave over fifty metres of water, but the raster
 * also holds ground that rises a hundred metres above the surface, so the domain has
 * a coast. The dry threshold is metres: A thinner film keeps a finite momentum, and its velocity grows until
 * the admissible step underflows. Cells over ground shallower than the coastal depth
 * go on the subcell scheme before they ever dry.
 */
bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                                   Q,
  const int                                                     numberOfStates,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) const {
  const double minimumWaterHeight = RuntimeParameters::getMinimumWaterHeight();
  if (numberOfStates == 1) {
    return ::applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, minimumWaterHeight);
  }

  const auto statistics = ::applications::exahype2::exaskred::limiting::
    collectCellStatistics<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, numberOfStates);
  return statistics.finite and statistics.minHeight > minimumWaterHeight
    and statistics.maxBathymetry <= -RuntimeParameters::getCoastalDepth();
}

void applications::exahype2::exaskred::ADERDGSolver::mapDmpObservables(
  double* const NOALIAS       observables,
  const double* const NOALIAS Q
) const {
  for (int observable = 0; observable < NumberOfDMPObservables; observable++) {
    observables[observable] = Q[observable];
  }
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
  const auto maxEigenvalueOfState = [](
                                      const double* const NOALIAS                  state,
                                      const tarch::la::Vector<DIMENSIONS, double>& facePosition,
                                      const tarch::la::Vector<DIMENSIONS, double>& faceSize,
                                      const double                                 time,
                                      const double                                 timeStepSize,
                                      const int                                    normal
                                    ) {
    return ::applications::exahype2::exaskred::shallowwater::maxEigenvalue<Shortcuts>(
      state,
      facePosition,
      faceSize,
      time,
      timeStepSize,
      normal,
      g,
      RuntimeParameters::getMinimumWaterHeight()
    );
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
      maxEigenvalueOfState,
      interfaceGravity,
      FL,
      FR
    );
}
