// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "internal.h"

#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"

double peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(
  const ::peano4::dataview::VertexMarker& marker
) {
  return peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(
  const ::peano4::dataview::CellMarker& marker
) {
  return peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  return peano4::particles::ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

double peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::dataview::VertexMarker& marker) {
  return peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::dataview::CellMarker& marker) {
  return peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector<DIMENSIONS, double>& h) {
  return peano4::particles::GrabOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

/**
 * Returns whether a particle with the given search radius fits into the cells
 * of the marker's level, which holds when twice the radius is at most the
 * smallest cell width.
 */
bool peano4::particles::fitsIntoLevel(const double searchRadius, const ::peano4::dataview::CellMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

bool peano4::particles::fitsIntoLevel(const double searchRadius, const ::peano4::dataview::VertexMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

/**
 * Returns the vertex of the cell around cellCentre that holds a particle at x.
 * The result carries one bit per axis, and bit d is set when the particle lies
 * at or beyond the cell centre along axis d.
 */
std::bitset<DIMENSIONS> peano4::particles::getParticleAssociationWithinCell(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre
) {
  std::bitset<DIMENSIONS> destAssociation;

  for (int d = 0; d < DIMENSIONS; d++) {
    destAssociation[d] = tarch::la::greaterEquals(x(d), cellCentre(d));
  }

  return destAssociation;
}
