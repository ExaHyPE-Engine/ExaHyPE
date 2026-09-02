// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "internal.h"

#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/CellMarker.h"

double peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const tarch::la::Vector< DIMENSIONS, double >& h) {
  return ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

double peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector< DIMENSIONS, double >& h) {
  return GrabOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

bool peano4::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::CellMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

bool peano4::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::VertexMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

/**
 * Find out which vertex should hold a particle.
 *
 * @param x          Position of the particle of interest.
 * @param cellCentre Centre of the cell, as returned by CellMarker::getX().
 * @return           Number of the vertex within that cell to which the
 *                   particle should be assigned. The result identifies
 *                   the vertex in z-order, i.e. 00 is the left bottom
 *                   vertex, 10 the right bottom one, and so forth. Use
 *                   to_ulong() to convert it.
 */
std::bitset<DIMENSIONS> peano4::particles::internal::getParticleAssociationWithinCell(
  const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& cellCentre
) {
  std::bitset<DIMENSIONS> destAssociation;

  // marker.getX() gives me the centre of the cell
  for (int d = 0; d < DIMENSIONS; d++) {
    destAssociation[d] = tarch::la::greaterEquals(x(d), cellCentre(d));
  }

  return destAssociation;
}
