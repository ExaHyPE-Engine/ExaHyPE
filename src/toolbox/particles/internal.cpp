#include "internal.h"

#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/CellMarker.h"

double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.getH());
}

double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const tarch::la::Vector< DIMENSIONS, double >& h) {
  return ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.getH());
}

double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector< DIMENSIONS, double >& h) {
  return GrabOwnershipSpatialSortingTolerance * tarch::la::max(h);
}

bool toolbox::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::CellMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

bool toolbox::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::VertexMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.getH()), 2.0 * searchRadius);
}

std::bitset<DIMENSIONS> toolbox::particles::internal::getParticleAssociationWithinCell(
  const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& cellCentre
) {
  std::bitset<DIMENSIONS> destAssociation;

  // marker.getX() gives me the centre of the cell
  for (int d = 0; d < DIMENSIONS; d++) {
    destAssociation[d] = tarch::la::greaterEquals(x(d), cellCentre(d));
  }

  return destAssociation;
}
