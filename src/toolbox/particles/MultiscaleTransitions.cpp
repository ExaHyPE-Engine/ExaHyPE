#include "MultiscaleTransitions.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/utils/Loop.h"


namespace {
  tarch::logging::Log _log("toolbox::particles");
} // namespace


double toolbox::particles::relativeSpatialOwnershipTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
}

double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker) {
  return GrabOwnershipSpatialSortingTolerance * tarch::la::max(marker.h());
}


double toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return GrabOwnershipSpatialSortingTolerance * tarch::la::max(marker.h());
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


toolbox::particles::ParticleReassociationInstruction toolbox::particles::liftParticleAssociatedWithVertex(
  bool isLocal, double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker marker
) {
  const double tolerance = internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  if (not isLocal) {
    return ParticleReassociationInstruction_Keep;
  }
  else if (internal::fitsIntoLevel(searchRadius, marker) and marker.isContainedInAdjacentCells(x, 0.5, tolerance)) {
    return ParticleReassociationInstruction_Keep;
  }
  else if (
    marker.isParentCellLocal()
    and
    peano4::datamanagement::CellMarker::isContained(x, marker.getInvokingParentCellsCentre(), marker.h() * 3.0, tolerance)
  ) {
    std::bitset<Dimensions> target = internal::getParticleAssociationWithinCell(x, marker.getInvokingParentCellsCentre());
    if (
      marker.isParentVertexLocal(target.to_ulong())
      and
      toolbox::particles::particleAssignedToVertexWillBeLocal( x, marker )
    ) {
      return target.to_ulong();
    } else {
      return ParticleReassociationInstruction_SieveGlobally;
    }
  }
  else {
    return ParticleReassociationInstruction_SieveGlobally;
  }
}


toolbox::particles::ParticleReassociationInstruction toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
  bool isLocal, double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::CellMarker& marker, int numberOfVertexWithinCell
) {
  if (not isLocal) {
    return ParticleReassociationInstruction_Keep;
  } else {
    const std::bitset<Dimensions> currentAssociation = numberOfVertexWithinCell;
    const std::bitset<Dimensions> destAssociation    = internal::getParticleAssociationWithinCell(x, marker.x());

    // marker.x() gives me the centre of the cell
    bool remainsOnSameLevel = internal::fitsIntoLevel(searchRadius, marker);
    for (int d = 0; d < Dimensions; d++) {
      remainsOnSameLevel &= tarch::la::smallerEquals(std::abs(x(d) - marker.x()(d)), marker.h()(d) * ReleaseOwnershipSpatialSortingTolerance);
    }

    if (remainsOnSameLevel and currentAssociation == destAssociation) {
      return ParticleReassociationInstruction_Keep;
    } else if (remainsOnSameLevel) {
      logDebug(
        "getParticleReassociationInstructionWithinCellWithIntraCellReassignment(...)",
        "reassign particle at "
          << x << " within cell " << marker.toString() << " which had been associated with vertex " << currentAssociation << " but belongs to " << destAssociation
      );
      return destAssociation.to_ulong();
    } else if (not marker.isParentLocal()) {
      return ParticleReassociationInstruction_SieveGlobally;
    } else {
      logDebug(
        "getParticleReassociationInstructionWithinCellWithIntraCellReassignment(...)",
        "lift particle at " << x << " from cell " << marker.toString() << " as difference is " << (x - marker.x())
      );

      const std::bitset<Dimensions> coarseDestAssociation = internal::getParticleAssociationWithinCell(x, marker.getInvokingParentCellsCentre());

      return coarseDestAssociation.to_ulong() + TwoPowerD;
    }
  }
  return -65536;
}


bool toolbox::particles::dropParticle(double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker) {
  bool result = internal::fitsIntoLevel(searchRadius, marker)
            and marker.isContainedInAdjacentCells(x, 0.5, internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE);

  assertion3(
    not (result and liftParticleAssociatedWithVertex(true, searchRadius, x, marker)==ParticleReassociationInstruction_SieveGlobally),
    searchRadius,
    x,
    marker.toString()
  );

  return result;
}


bool toolbox::particles::sieveParticle(double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker) {
  bool belongsIntoNeighbouringCells = marker.isContainedInAdjacentCells(
    x, 0.5, internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  bool fitsIntoCurrentMeshLevel = internal::fitsIntoLevel(searchRadius, marker);
  bool willNotBeDroppedFurther  = not internal::fitsIntoLevel(searchRadius * 3.0, marker) or not marker.hasBeenRefined();

  return belongsIntoNeighbouringCells and fitsIntoCurrentMeshLevel and willNotBeDroppedFurther;
}


bool toolbox::particles::particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::CellMarker& marker) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}


bool toolbox::particles::particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::VertexMarker& marker) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}


std::bitset<TwoPowerD> toolbox::particles::getAdjacentCellsOwningParticle(const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker) {
  std::bitset<TwoPowerD> result = 0;

  dfor2(k) {
    tarch::la::Vector<Dimensions, int> adjacentCell(k);
    tarch::la::Vector<Dimensions, double>
      adjacentCellCentre = marker.x() + tarch::la::multiplyComponents(tarch::la::convertScalar<double>(adjacentCell) + tarch::la::Vector<Dimensions, double>(-0.5), marker.h());
    result[kScalar]      = peano4::datamanagement::CellMarker::isContained(
      x, adjacentCellCentre, marker.h(), internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    );
  }
  enddforx

    return result;
}
