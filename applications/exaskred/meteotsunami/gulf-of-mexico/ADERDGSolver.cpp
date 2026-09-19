// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AlbersEqualArea.h"
#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Forcing.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"

namespace {
  // The raster carries the projection it was sampled on, so the scenario reads
  // geographic coordinates through the same one.
  applications::exahype2::exaskred::projection::AlbersEqualArea gulfProjection() {
    namespace exaskred = applications::exahype2::exaskred;
    return {
      .referenceLongitude     = exaskred::RuntimeParameters::getProjectionCentralMeridian(),
      .referenceLatitude      = exaskred::RuntimeParameters::getProjectionOriginLatitude(),
      .firstStandardParallel  = exaskred::RuntimeParameters::getProjectionFirstParallel(),
      .secondStandardParallel = exaskred::RuntimeParameters::getProjectionSecondParallel(),
      .semiMajorAxis          = exaskred::RuntimeParameters::getProjectionSemiMajorAxis(),
      .eccentricity           = exaskred::RuntimeParameters::getProjectionEccentricity()
    };
  }

  // The storm centre runs along the great circle between the two track points at a
  // fixed speed, and the disturbance is carried rigidly with it.
  std::pair<double, double> stormCentre(const double t) {
    namespace exaskred    = applications::exahype2::exaskred;
    const auto projection = gulfProjection();
    const auto start      = projection.forward(
      exaskred::RuntimeParameters::getTrackStartLongitude(),
      exaskred::RuntimeParameters::getTrackStartLatitude()
    );
    const auto end = projection.forward(
      exaskred::RuntimeParameters::getTrackEndLongitude(),
      exaskred::RuntimeParameters::getTrackEndLatitude()
    );
    const double runX      = end.first - start.first;
    const double runY      = end.second - start.second;
    const double length    = std::hypot(runX, runY);
    const double travelled = length > 0.0 ? exaskred::RuntimeParameters::getTrackSpeed() * t / length : 0.0;
    return {start.first + travelled * runX, start.second + travelled * runY};
  }

  double trackBearing() {
    namespace exaskred    = applications::exahype2::exaskred;
    const auto projection = gulfProjection();
    const auto start      = projection.forward(
      exaskred::RuntimeParameters::getTrackStartLongitude(),
      exaskred::RuntimeParameters::getTrackStartLatitude()
    );
    const auto end = projection.forward(
      exaskred::RuntimeParameters::getTrackEndLongitude(),
      exaskred::RuntimeParameters::getTrackEndLatitude()
    );
    return std::atan2(end.second - start.second, end.first - start.first);
  }

  /**
   * Pressure anomaly of the disturbance and its derivatives in the frame that
   * travels with it.
   *
   * The anomaly is a Gaussian multiplied by the along-track offset, which gives it
   * a trough ahead of the centre and a crest behind. The two along-track widths
   * differ, so the shape is steeper on its leading side.
   */
  struct PressureAnomaly final {
    double value{0.0};
    double alongTrackDerivative{0.0};
    double acrossTrackDerivative{0.0};
  };

  PressureAnomaly pressureAnomaly(const double alongTrack, const double acrossTrack) {
    namespace exaskred      = applications::exahype2::exaskred;
    const double crossWidth = exaskred::RuntimeParameters::getPressureCrossTrackWidth();
    const double amplitude  = alongTrack < 0.0 ? exaskred::RuntimeParameters::getPressureLeadingAmplitude()
                                               : exaskred::RuntimeParameters::getPressureTrailingAmplitude();
    const double width      = alongTrack < 0.0 ? exaskred::RuntimeParameters::getPressureLeadingWidth()
                                               : exaskred::RuntimeParameters::getPressureTrailingWidth();

    const double scaledAcross = acrossTrack / crossWidth;
    const double scaledAlong  = alongTrack / width;
    const double envelope     = std::exp(-scaledAcross * scaledAcross - scaledAlong * scaledAlong);

    PressureAnomaly anomaly;
    anomaly.value                 = -amplitude * alongTrack * envelope;
    anomaly.alongTrackDerivative  = -amplitude * envelope * (1.0 - 2.0 * alongTrack * alongTrack / (width * width));
    anomaly.acrossTrackDerivative = amplitude * alongTrack * envelope * 2.0 * acrossTrack / (crossWidth * crossWidth);
    return anomaly;
  }

  /**
   * Gradient of the atmospheric pressure in metric coordinates.
   *
   * The anomaly is defined in a frame that travels with the storm and is rotated
   * onto its track, both of which are linear maps of the geographic offsets. Their
   * derivatives are therefore constant, and the only step that needs the shape of
   * the earth is the last one, where the projection's inverse Jacobian turns a
   * geographic gradient into a metric one.
   */
  tarch::la::Vector<DIMENSIONS, double> atmosphericPressureGradient(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const double                                 t
  ) {
    const auto projection       = gulfProjection();
    const auto here             = projection.inverse(x(0), x(1));
    const auto centre           = stormCentre(t);
    const auto centreGeographic = projection.inverse(centre.first, centre.second);

    const double bearingSine   = std::sin(trackBearing());
    const double bearingCosine = std::cos(trackBearing());
    const double convergence   = std::cos(centreGeographic.second * std::numbers::pi_v<double> / 180.0);

    const double longitudeOffset = std::remainder(here.first - centreGeographic.first, 360.0) * convergence;
    const double latitudeOffset  = here.second - centreGeographic.second;
    const double alongTrack      = longitudeOffset * bearingCosine + latitudeOffset * bearingSine;
    const double acrossTrack     = -longitudeOffset * bearingSine + latitudeOffset * bearingCosine;

    const auto anomaly = pressureAnomaly(std::abs(convergence) > 1e-12 ? alongTrack / convergence : 0.0, acrossTrack);

    // Derivatives of the travelling frame with respect to longitude and latitude.
    const double alongTrackByLongitude  = bearingCosine;
    const double alongTrackByLatitude   = std::abs(convergence) > 1e-12 ? bearingSine / convergence : 0.0;
    const double acrossTrackByLongitude = -convergence * bearingSine;
    const double acrossTrackByLatitude  = bearingCosine;

    const double byLongitude =
      anomaly.alongTrackDerivative * alongTrackByLongitude + anomaly.acrossTrackDerivative * acrossTrackByLongitude;
    const double byLatitude =
      anomaly.alongTrackDerivative * alongTrackByLatitude + anomaly.acrossTrackDerivative * acrossTrackByLatitude;

    const auto jacobian = projection.inverseJacobian(x(0), x(1));
    return {
      byLongitude * jacobian(0, 0) + byLatitude * jacobian(1, 0),
      byLongitude * jacobian(0, 1) + byLatitude * jacobian(1, 1)
    };
  }

  tarch::la::Vector<DIMENSIONS, double> clampToRaster(const tarch::la::Vector<DIMENSIONS, double>& x) {
    const auto offset = applications::exahype2::exaskred::RuntimeParameters::getBathymetryOffset();
    const auto size   = applications::exahype2::exaskred::RuntimeParameters::getBathymetrySize();
    tarch::la::Vector<DIMENSIONS, double> clamped;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      clamped(dimension) = std::clamp(x(dimension), offset(dimension), offset(dimension) + size(dimension));
    }
    return clamped;
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                                         Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  static tarch::reader::NetCDFFieldSampler<2> bathymetrySampler(
    {{"bathymetry", RuntimeParameters::getBathymetryFile(), "Band1"}}
  );

  const double sampled = bathymetrySampler.sampleBilinear("bathymetry", clampToRaster(x));
  // The raster marks the cells it holds no survey for with a fill value far below
  // any real sea floor. Those cells become open ocean of the reference depth.
  const double referenceDepth = RuntimeParameters::getReferenceOceanDepth();
  const bool   surveyed       = std::isfinite(sampled) and sampled >= -RuntimeParameters::getDeepestSeaFloor();
  const double bathymetry     = surveyed ? sampled : -referenceDepth;

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
  [[maybe_unused]] const int                                    normal
) {
  Qoutside[Shortcuts::h]      = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu]     = 0.0;
  Qoutside[Shortcuts::hu + 1] = 0.0;
  Qoutside[Shortcuts::b]      = Qinside[Shortcuts::b];
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
 * The basin spans thirteen degrees of latitude, so the Coriolis parameter follows
 * the latitude of the point.
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

  const double latitude = gulfProjection().inverse(x(0), x(1)).second;
  const double coriolis = ::applications::exahype2::exaskred::forcing::coriolisParameterOnSphere(
    latitude,
    RuntimeParameters::getEarthAngularVelocity()
  );
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
 * A cell is troubled over the shelf and the coast, where the Galerkin face solver
 * has no wet and dry structure to fall back on.
 *
 * The augmented Riemann solver the subcell scheme uses carries the inundation
 * cases, so a cell has to reach that scheme before its water column thins, not
 * after.
 */
bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                  Q,
  const int                                    numberOfStates,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) const {
  const double minimumWaterHeight = RuntimeParameters::getMinimumWaterHeight();
  if (numberOfStates == 1) {
    return ::applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, minimumWaterHeight);
  }

  // A boundary condition is not a solution of the equations, and beyond the survey
  // the bed continues its rim at a constant depth. Both stand on the well-balanced
  // Rusanov scheme.
  const auto rasterOffset = RuntimeParameters::getBathymetryOffset();
  const auto rasterSize   = RuntimeParameters::getBathymetrySize();
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    if (x(dimension) - 0.5 * h(dimension) <= DomainOffset[dimension]
        or x(dimension) + 0.5 * h(dimension) >= DomainOffset[dimension] + DomainSize[dimension]
        or x(dimension) - 0.5 * h(dimension) <= rasterOffset(dimension)
        or x(dimension) + 0.5 * h(dimension) >= rasterOffset(dimension) + rasterSize(dimension)) {
      return false;
    }
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
