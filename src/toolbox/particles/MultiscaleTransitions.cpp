// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiscaleTransitions.h"

#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"

/**
 * Returns the tolerance by which a cell's area grows before it owns the particles that
 * reside within it.
 */
double toolbox::particles::relativeSpatialOwnershipTolerance(const peano4::datamanagement::CellMarker& marker) {
  return internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
}

toolbox::particles::ParticleReassociationInstruction toolbox::particles::liftParticleAssociatedWithVertex(
  const bool                                   isLocal,
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker   marker
) {
  const double tolerance =
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  const auto image = toolbox::particles::periodicImageTowardsVertex(x, marker);
  if (not isLocal) {
    return ParticleReassociationInstructionKeep;
  } else if (internal::fitsIntoLevel(searchRadius, marker)
             and marker.isContainedInAdjacentCells(image, 0.5, tolerance)) {
    return ParticleReassociationInstructionKeep;
  } else if (marker.isParentCellLocal()
             and peano4::datamanagement::CellMarker::isContained(
               image,
               marker.getInvokingParentCellsCentre(),
               marker.getH() * 3.0,
               tolerance
             )) {
    const std::bitset<DIMENSIONS> target =
      internal::getParticleAssociationWithinCell(image, marker.getInvokingParentCellsCentre());
    if (marker.isParentVertexLocal(target.to_ulong())
        and toolbox::particles::particleAssignedToVertexWillBeLocal(image, marker)) {
      return target.to_ulong();
    } else {
      return ParticleReassociationInstructionSieveGlobally;
    }
  } else {
    return ParticleReassociationInstructionSieveGlobally;
  }
}

toolbox::particles::ParticleReassociationInstruction toolbox::particles::
  getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
    const bool                                   isLocal,
    const double                                 searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::datamanagement::CellMarker&    marker,
    const int                                    numberOfVertexWithinCell
  ) {
  if (not isLocal) {
    return ParticleReassociationInstructionKeep;
  } else {
    const std::bitset<DIMENSIONS> currentAssociation = numberOfVertexWithinCell;
    const std::bitset<DIMENSIONS> destAssociation    = internal::getParticleAssociationWithinCell(x, marker.getX());

    bool remainsOnSameLevel = internal::fitsIntoLevel(searchRadius, marker);
    for (int d = 0; d < DIMENSIONS; d++) {
      remainsOnSameLevel &= tarch::la::smallerEquals(
        std::abs(x(d) - marker.getX()(d)),
        marker.getH()(d) * ReleaseOwnershipSpatialSortingTolerance
      );
    }

    if (remainsOnSameLevel and currentAssociation == destAssociation) {
      return ParticleReassociationInstructionKeep;
    } else if (remainsOnSameLevel) {
      logDebug(
        "reassign particle at " << x << " within cell " << marker.toString()
                                << " which had been associated with vertex " << currentAssociation << " but belongs to "
                                << destAssociation
      );
      return destAssociation.to_ulong();
    } else if (not marker.isParentLocal()) {
      return ParticleReassociationInstructionSieveGlobally;
    } else {
      logDebug(
        "lift particle at " << x << " from cell " << marker.toString() << " as difference is " << (x - marker.getX())
      );

      const std::bitset<DIMENSIONS> coarseDestAssociation =
        internal::getParticleAssociationWithinCell(x, marker.getInvokingParentCellsCentre());

      return coarseDestAssociation.to_ulong() + TWO_POWER_D;
    }
  }
}

/**
 * Returns whether the particle at x drops onto the vertex the marker names.
 */
bool toolbox::particles::dropParticle(
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  const bool result = internal::fitsIntoLevel(searchRadius, marker)
    and marker.isContainedInAdjacentCells(
      toolbox::particles::periodicImageTowardsVertex(x, marker),
      0.5,
      internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    );

  assertion(
    not result
      or liftParticleAssociatedWithVertex(true, searchRadius, x, marker)
        != ParticleReassociationInstructionSieveGlobally,
    searchRadius,
    x,
    marker.toString()
  );

  return result;
}

bool toolbox::particles::sieveParticle(
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  const bool belongsIntoNeighbouringCells = marker.isContainedInAdjacentCells(
    toolbox::particles::periodicImageTowardsVertex(x, marker),
    0.5,
    internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  const bool fitsIntoCurrentMeshLevel = internal::fitsIntoLevel(searchRadius, marker);
  const bool willNotBeDroppedFurther =
    not internal::fitsIntoLevel(searchRadius * 3.0, marker) or not marker.hasBeenRefined();

  return belongsIntoNeighbouringCells and fitsIntoCurrentMeshLevel and willNotBeDroppedFurther;
}

bool toolbox::particles::particleWillBeDroppedFurther(
  const double                              searchRadius,
  const peano4::datamanagement::CellMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}

bool toolbox::particles::particleWillBeDroppedFurther(
  const double                                searchRadius,
  const peano4::datamanagement::VertexMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}
