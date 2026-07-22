// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
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

/**
 * Mirror a particle along the periodic domain boundaries.
 *
 * We check for every boundary whether it is periodic. Furthermore, we check
 * whether the particle would be already contained within a vertex along
 * each dimension. This is necessary for vertices in corners of the domain:
 * In these cases, we need to mirror the particle only along the boundary
 * that isn't contained in the vertex. Otherwise, the particle is displaced
 * too far away. For a vertex at (0, 0), a particle at (0.95, 0.) has to be
 * displaced to (-0.05, 0.) and not to (-0.05, -1.).
 *
 * @param x Position of the particle as received along a periodic boundary
 *   exchange.
 * @param marker Marker of vertex which triggers the receive, i.e. the one
 *   to which the incoming particle at x will be attached to.
 * @return Position of particle as "seen" from receiving side
 */
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

/**
 * Applies the periodic boundary condition on particle coordinates `x`.
 *
 * In mirrorParticleAlongPeriodicDomains(), we check whether a vertex is
 * located on the boundary, and if it is, apply the boundary conditions
 * such that the vertex would "see" the particles.
 * In contrast, here we blindly apply the periodic boundary conditions
 * to a particle. This function is intended to be run on global particle
 * lists, such as is used e.g. in exchangeSieveListsGlobally().
 */
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
