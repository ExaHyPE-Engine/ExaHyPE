#include "particles.h"

#include "MultiscaleTransitions.h"

#include "peano4/Init.h"

bool toolbox::particles::particleAssignedToVertexWillBeLocal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  std::bitset<TWO_POWER_D> adjacentCellWhichHostsParticle = getAdjacentCellsOwningParticle(x, marker);
  return adjacentCellWhichHostsParticle.to_ulong() & marker.areAdjacentCellsLocal().to_ulong();
}

tarch::la::Vector<DIMENSIONS, double> toolbox::particles::mirrorParticleAlongPeriodicDomains(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker,
  const tarch::la::Vector<DIMENSIONS, double>  domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>  domainSize,
  const std::bitset<DIMENSIONS>                periodicBC
) {
  // Create a copy of the input position vector
  tarch::la::Vector<DIMENSIONS, double> mirroredPosition = x;

  // Check each dimension for periodic boundary conditions
  for (int d = 0; d < DIMENSIONS; d++) {
    // Skip non-periodic dimensions
    if (!periodicBC[d])
      continue;

    // Calculate domain boundaries for this dimension
    const double lowerBoundary = domainOffset[d];
    const double upperBoundary = domainOffset[d] + domainSize[d];

    // Apply mirroring for particles outside the domain boundaries
    if (tarch::la::smallerEquals(x(d), lowerBoundary)) {
      mirroredPosition(d) += domainSize(d);
    } else if (tarch::la::greaterEquals(x(d), upperBoundary)) {
      mirroredPosition(d) -= domainSize(d);
    }
  }

  return mirroredPosition;
}

tarch::la::Vector<DIMENSIONS, double> toolbox::particles::applyPeriodicBoundaryConditions(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>  domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>  domainSize,
  const std::bitset<DIMENSIONS>                periodicBC
) {
  tarch::la::Vector<DIMENSIONS, double> result = x;

  for (int d = 0; d < DIMENSIONS; d++) {
    if (periodicBC[d]) {
      if (tarch::la::smaller(result(d), domainOffset(d))) {
        result(d) += domainSize(d);
      }
      if (tarch::la::greater(result(d), domainOffset(d) + domainSize(d))) {
        result(d) -= domainSize(d);
      }
    }
  }

  return result;
}
