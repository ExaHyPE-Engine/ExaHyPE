#include "internal.h"


#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/CellMarker.h"


double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.h());
}


double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeReleaseOwnershipSpatialSortingTolerance(marker.h());
}


double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const tarch::la::Vector< Dimensions, double >& h) {
  return ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(h);
}


double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.h());
}


double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return relativeGrabOwnershipSpatialSortingTolerance(marker.h());
}


double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector< Dimensions, double >& h) {
  return GrabOwnershipSpatialSortingTolerance * tarch::la::max(h);
}


bool toolbox::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::CellMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.h()), 2.0 * searchRadius);
}


bool toolbox::particles::internal::fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::VertexMarker& marker) {
  return tarch::la::greaterEquals(tarch::la::min(marker.h()), 2.0 * searchRadius);
}


std::bitset<Dimensions> toolbox::particles::internal::getParticleAssociationWithinCell(
  const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& cellCentre
) {
  std::bitset<Dimensions> destAssociation;

  // marker.x() gives me the centre of the cell
  for (int d = 0; d < Dimensions; d++) {
    destAssociation[d] = tarch::la::greaterEquals(x(d), cellCentre(d));
  }

  return destAssociation;
}
