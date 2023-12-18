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


toolbox::particles::ParticleReassociationInstruction toolbox::particles::liftParticleAssociatedWithVertex(
  bool                                         isLocal,
  double                                       searchRadius,
  const tarch::la::Vector<Dimensions,double>&  x,
  const peano4::datamanagement::VertexMarker   marker
) {
  if (not isLocal) {
    return ParticleReassociationInstruction_Keep;
  }
  else if (
    tarch::la::min(marker.h()) * MultilevelSortingTolerance >= 2.0 * searchRadius
    and
    marker.isContainedInAdjacentCells( x, 0.5, relativeSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE)
  ) {
    return ParticleReassociationInstruction_Keep;
  } else if (not marker.isParentCellLocal()) {
    return ParticleReassociationInstruction_SieveGlobally;
  } else {
    std::bitset<Dimensions> target;
    for (int d = 0; d < Dimensions; d++) {
      target[d] = marker.getRelativePositionWithinFatherCell()(d) >= 2;
    }

    if (marker.isParentVertexLocal(target.to_ulong())) {
      return target.to_ulong();
    } else {
      return ParticleReassociationInstruction_SieveGlobally;
    }
  }
  return -65536;

}
