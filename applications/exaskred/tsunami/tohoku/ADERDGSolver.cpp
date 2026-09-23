// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Limiting.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/reader/NetCDFFieldSampler.h"

namespace {
  bool displacementContainsPoint(const double sampleX, const double sampleY) {
    const auto   displacementSize     = applications::exahype2::exaskred::RuntimeParameters::getDisplacementSize();
    const auto   displacementOffset   = applications::exahype2::exaskred::RuntimeParameters::getDisplacementOffset();
    const auto   displacementPosition = applications::exahype2::exaskred::RuntimeParameters::getDisplacementPosition();
    const double fileX                = sampleX - displacementPosition(0);
    const double fileY                = sampleY - displacementPosition(1);
    return fileX >= displacementOffset(0) and fileX <= displacementOffset(0) + displacementSize(0)
      and fileY >= displacementOffset(1) and fileY <= displacementOffset(1) + displacementSize(1);
  }

  double sampleDisplacement(
    tarch::reader::NetCDFFieldSampler<2>& displacementSampler,
    const double                          sampleX,
    const double                          sampleY
  ) {
    if (not displacementContainsPoint(sampleX, sampleY)) {
      return 0.0;
    }
    const auto displacementPosition = applications::exahype2::exaskred::RuntimeParameters::getDisplacementPosition();
    return displacementSampler.sampleBilinear(
      "displacement",
      {sampleX - displacementPosition(0), sampleY - displacementPosition(1)}
    );
  }

  // The displacement raster resolves scales far below the mesh, and sampling it
  // pointwise aliases those scales into the polynomial basis. A nine-point
  // low-pass over the source's effective radius, shrunk by the cell footprint,
  // hands every resolution the same integrated source.
  double sampleFilteredDisplacement(
    tarch::reader::NetCDFFieldSampler<2>&        displacementSampler,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) {
    const double filterCellH0 = std::max(h(0), applications::exahype2::exaskred::sourceFilterMinimumCellH);
    const double filterCellH1 = std::max(h(1), applications::exahype2::exaskred::sourceFilterMinimumCellH);
    const double cellFootprintRadiusSquared = filterCellH0 * filterCellH0 + filterCellH1 * filterCellH1;
    const double sourceRadiusSquared        = std::max(
      applications::exahype2::exaskred::sourceLowPassEffectiveRadius
          * applications::exahype2::exaskred::sourceLowPassEffectiveRadius
        - cellFootprintRadiusSquared,
      0.0
    );
    const double sourceRadius = std::sqrt(sourceRadiusSquared);
    return (4.0 * sampleDisplacement(displacementSampler, x(0), x(1))
            + 2.0 * sampleDisplacement(displacementSampler, x(0) - sourceRadius, x(1))
            + 2.0 * sampleDisplacement(displacementSampler, x(0) + sourceRadius, x(1))
            + 2.0 * sampleDisplacement(displacementSampler, x(0), x(1) - sourceRadius)
            + 2.0 * sampleDisplacement(displacementSampler, x(0), x(1) + sourceRadius)
            + sampleDisplacement(displacementSampler, x(0) - sourceRadius, x(1) - sourceRadius)
            + sampleDisplacement(displacementSampler, x(0) + sourceRadius, x(1) - sourceRadius)
            + sampleDisplacement(displacementSampler, x(0) - sourceRadius, x(1) + sourceRadius)
            + sampleDisplacement(displacementSampler, x(0) + sourceRadius, x(1) + sourceRadius))
      / 16.0;
  }
} // namespace

void applications::exahype2::exaskred::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
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

  const auto bathymetryBeforeEarthquake = bathymetrySampler.sampleBilinear("bathymetry", {x(0), x(1)});
  const auto displacement               = sampleFilteredDisplacement(displacementSampler, x, h);
  const auto bathymetryAfterEarthquake  = bathymetryBeforeEarthquake + displacement;

  Q[Shortcuts::h]      = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[Shortcuts::hu]     = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = bathymetryAfterEarthquake;
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

// A cell is troubled at the physical boundary, in dry or coastal water, or when
// its free surface varies more than the scale-aware thresholds allow. The
// displacement region additionally bounds the absolute surface amplitude, so
// the earthquake source starts on the subcell scheme.
bool applications::exahype2::exaskred::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                  Q,
  const int                                    numberOfStates,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  if (numberOfStates == 1) {
    return ::applications::exahype2::exaskred::limiting::
      isAdmissibleState<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, 0.0);
  }
  const bool touchesPhysicalBoundary = x(0) - 0.5 * h(0) <= DomainOffset[0]
    or x(0) + 0.5 * h(0) >= DomainOffset[0] + DomainSize[0] or x(1) - 0.5 * h(1) <= DomainOffset[1]
    or x(1) + 0.5 * h(1) >= DomainOffset[1] + DomainSize[1];
  if (touchesPhysicalBoundary) {
    return false;
  }
  const auto statistics = ::applications::exahype2::exaskred::limiting::
    collectCellStatistics<Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(Q, numberOfStates);
  if (
    not statistics.finite or statistics.minHeight <= hThreshold
    or statistics.maxBathymetry > -limiterBathymetryThreshold
  ) {
    return false;
  }
  const double freeSurfaceSpan = statistics.maxFreeSurface - statistics.minFreeSurface;
  const double localDepthScale = std::max(statistics.maxHeight, hThreshold);
  const double referencePatchH = std::min(DomainSize[0], DomainSize[1]) / 81.0;
  const double patchSizeScale  = std::max(h(0), h(1)) / referencePatchH;
  if (freeSurfaceSpan > limiterRelativeFreeSurfaceVariation * localDepthScale * patchSizeScale) {
    return false;
  }
  if (statistics.minFreeSurface < -limiterRelativeNegativeFreeSurfaceVariation * localDepthScale) {
    return false;
  }
  const auto displacementSize     = ::applications::exahype2::exaskred::RuntimeParameters::getDisplacementSize();
  const auto displacementOffset   = ::applications::exahype2::exaskred::RuntimeParameters::getDisplacementOffset();
  const auto displacementPosition = ::applications::exahype2::exaskred::RuntimeParameters::getDisplacementPosition();
  const bool overlapsDisplacement = x(0) + 0.5 * h(0) >= displacementPosition(0) + displacementOffset(0)
    and x(0) - 0.5 * h(0) <= displacementPosition(0) + displacementOffset(0) + displacementSize(0)
    and x(1) + 0.5 * h(1) >= displacementPosition(1) + displacementOffset(1)
    and x(1) - 0.5 * h(1) <= displacementPosition(1) + displacementOffset(1) + displacementSize(1);
  if (
    overlapsDisplacement
    and std::max(std::abs(statistics.minFreeSurface), std::abs(statistics.maxFreeSurface))
      > limiterRelativeSourceFreeSurfaceAmplitude * localDepthScale
  ) {
    return false;
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
