#include "MultiscaleTransitions.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"


double toolbox::particles::relativeSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return SpatialSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::relativeSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return SpatialSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::relativeMultilevelSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return MultilevelSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::relativeMultilevelSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return MultilevelSortingTolerance * tarch::la::max(marker.h());
}
