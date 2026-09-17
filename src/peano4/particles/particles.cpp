// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "particles.h"

#include "peano4/topology/TopologyManager.h"

/**
 * Returns the periodic image of x that lies nearest to referencePosition. On a
 * periodic axis the image lies within half the domain width of the reference.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::periodicImageTowards(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& referencePosition,
  const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  tarch::la::Vector<DIMENSIONS, double> image = x;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (periodicBC[axis]) {
      assertion(domainWidth(axis) > 0.0, domainWidth, axis);
      const double halfWidth = 0.5 * domainWidth(axis);
      if (image(axis) - referencePosition(axis) > halfWidth) {
        image(axis) -= domainWidth(axis);
      } else if (referencePosition(axis) - image(axis) > halfWidth) {
        image(axis) += domainWidth(axis);
      }
    }
  }
  return image;
}

/** Returns the periodic image of x nearest to referencePosition within the domain the topology holds. */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::periodicImageTowards(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& referencePosition
) {
  const auto& topology = peano4::topology::TopologyManager::getInstance();
  if (topology.getPeriodicBC().none()) {
    return x;
  }
  return peano4::particles::periodicImageTowards(
    x,
    referencePosition,
    topology.getDomainWidth(),
    topology.getPeriodicBC()
  );
}

/**
 * Returns the periodic image of x nearest to the vertex the marker selects. A
 * periodic wall vertex is one entity whose marker carries the position of the
 * invoking cell's corner, so a particle at the opposite wall lies in the
 * adjacent cells as this image.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::periodicImageTowardsVertex(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker&        marker
) {
  return peano4::particles::periodicImageTowards(x, marker.getX());
}

/**
 * Returns the periodic image of x nearest to the centre of the cell of marker.
 * A cell at a periodic wall holds the wall vertex, whose particles carry
 * positions from either wall, so the cell reads a particle through this image.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::periodicImageTowardsCell(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::CellMarker&          marker
) {
  return peano4::particles::periodicImageTowards(x, marker.getX());
}

/** Returns whether the domain the topology holds has a periodic axis. */
bool peano4::particles::domainHasPeriodicAxes() {
  return peano4::topology::TopologyManager::getInstance().getPeriodicBC().any();
}

/**
 * Returns the adjacent cells that claim the particle at x. The cells overlap by
 * the sorting tolerance, so more than one of them can claim the same particle.
 */
std::bitset<TWO_POWER_D> peano4::particles::getAdjacentCellsOwningParticle(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker&        marker
) {
  std::bitset<TWO_POWER_D>                    result         = 0;
  const auto                                  image          = peano4::particles::periodicImageTowardsVertex(x, marker);
  const tarch::la::Vector<DIMENSIONS, double> selectedVertex = marker.getX();
  const tarch::la::Vector<DIMENSIONS, double> h              = marker.getH();
  const double tolerance = peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker)
    * tarch::la::NUMERICAL_ZERO_DIFFERENCE;

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, const int kScalar) {
    const tarch::la::Vector<DIMENSIONS, int> adjacentCell(k);
    tarch::la::Vector<DIMENSIONS, double>    adjacentCellCentre(0.0);
    for (int d = 0; d < DIMENSIONS; d++) {
      adjacentCellCentre(d) = selectedVertex(d) + (static_cast<double>(adjacentCell(d)) - 0.5) * h(d);
    }
    result[kScalar] = peano4::dataview::CellMarker::isContained(image, adjacentCellCentre, h, tolerance);
  });

  return result;
}

bool peano4::particles::particleAssignedToVertexWillBeLocal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker&        marker
) {
  const std::bitset<TWO_POWER_D> adjacentCellWhichHostsParticle =
    peano4::particles::getAdjacentCellsOwningParticle(x, marker);
  return (adjacentCellWhichHostsParticle & marker.areAdjacentCellsLocal()).any();
}

/**
 * Applies the periodic boundary condition on the particle coordinates x and
 * returns them. The routine reads no vertex, so a caller applies it to a
 * global particle list such as the one of exchangeSieveListsGlobally().
 */
tarch::la::Vector<DIMENSIONS, double> peano4::particles::applyPeriodicBoundaryConditions(
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
