// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiscaleTransitions.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"

namespace {
  /** Width of a cell divided by the width of one of its children. */
  constexpr double MeshWidthRatio = peano4::sfc::ActiveCurve::SubdivisionFactor;
} // namespace

/**
 * Returns the length by which the cell of the marker grows along every axis before it
 * releases the particles it holds.
 */
double peano4::particles::relativeSpatialOwnershipTolerance(const peano4::dataview::CellMarker& marker) {
  return peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(marker)
    * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
}

peano4::particles::ParticleReassociationInstruction peano4::particles::liftParticleAssociatedWithVertex(
  const bool                                   isLocal,
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker         marker
) {
  const double tolerance =
    peano4::particles::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  const auto image = peano4::particles::periodicImageTowardsVertex(x, marker);
  if (not isLocal) {
    return ParticleReassociationInstructionKeep;
  } else if (
    peano4::particles::fitsIntoLevel(searchRadius, marker) and marker.isContainedInAdjacentCells(image, 0.5, tolerance)
  ) {
    return ParticleReassociationInstructionKeep;
  } else if (
    marker.isParentCellLocal()
    and peano4::dataview::CellMarker::isContained(
      image,
      marker.getInvokingParentCellsCentre(),
      marker.getH() * MeshWidthRatio,
      tolerance
    )
  ) {
    const std::bitset<DIMENSIONS> target =
      peano4::particles::getParticleAssociationWithinCell(image, marker.getInvokingParentCellsCentre());
    if (
      marker.isParentVertexLocal(target.to_ulong())
      and peano4::particles::particleAssignedToVertexWillBeLocal(image, marker)
    ) {
      return target.to_ulong();
    } else {
      return ParticleReassociationInstructionSieveGlobally;
    }
  } else {
    return ParticleReassociationInstructionSieveGlobally;
  }
}

peano4::particles::ParticleReassociationInstruction peano4::particles::
  getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
    const bool                                   isLocal,
    const double                                 searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::CellMarker&          marker,
    const int                                    numberOfVertexWithinCell
  ) {
  if (not isLocal) {
    return ParticleReassociationInstructionKeep;
  } else {
    const std::bitset<DIMENSIONS> currentAssociation = numberOfVertexWithinCell;
    const std::bitset<DIMENSIONS> destAssociation =
      peano4::particles::getParticleAssociationWithinCell(x, marker.getX());

    bool remainsOnSameLevel = peano4::particles::fitsIntoLevel(searchRadius, marker);
    for (int d = 0; d < DIMENSIONS; d++) {
      remainsOnSameLevel &= tarch::la::smallerEquals(
        std::abs(x(d) - marker.getX()(d)),
        marker.getH()(d) * peano4::particles::ReleaseOwnershipSpatialSortingTolerance
      );
    }

    if (remainsOnSameLevel and currentAssociation == destAssociation) {
      return ParticleReassociationInstructionKeep;
    } else if (remainsOnSameLevel) {
      logDebug(
        "Reassign particle at " << x << " within cell " << marker.toString()
                                << " which had been associated with vertex " << currentAssociation << " but belongs to "
                                << destAssociation
      );
      return destAssociation.to_ulong();
    } else if (not marker.isParentLocal()) {
      return ParticleReassociationInstructionSieveGlobally;
    } else {
      logDebug(
        "Lift particle at " << x << " from cell " << marker.toString() << " as difference is " << (x - marker.getX())
      );

      const std::bitset<DIMENSIONS> coarseDestAssociation =
        peano4::particles::getParticleAssociationWithinCell(x, marker.getInvokingParentCellsCentre());

      return coarseDestAssociation.to_ulong() + peano4::VerticesPerCell;
    }
  }
}

/**
 * Returns whether the particle at x drops onto the vertex the marker names.
 */
bool peano4::particles::dropParticle(
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker&        marker
) {
  const bool result = peano4::particles::fitsIntoLevel(searchRadius, marker)
    and marker.isContainedInAdjacentCells(
      peano4::particles::periodicImageTowardsVertex(x, marker),
      0.5,
      peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    );

#if PEANO_ASSERTIONS
  const bool dropAgreesWithLift = not result
    or liftParticleAssociatedWithVertex(true, searchRadius, x, marker) != ParticleReassociationInstructionSieveGlobally;
  assertion(dropAgreesWithLift, searchRadius, x, marker.toString());
#endif

  return result;
}

bool peano4::particles::sieveParticle(
  const double                                 searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::dataview::VertexMarker&        marker
) {
  const bool belongsIntoNeighbouringCells = marker.isContainedInAdjacentCells(
    peano4::particles::periodicImageTowardsVertex(x, marker),
    0.5,
    peano4::particles::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  const bool fitsIntoCurrentMeshLevel = peano4::particles::fitsIntoLevel(searchRadius, marker);
  const bool willNotBeDroppedFurther =
    not peano4::particles::fitsIntoLevel(searchRadius * MeshWidthRatio, marker) or not marker.hasBeenRefined();

  return belongsIntoNeighbouringCells and fitsIntoCurrentMeshLevel and willNotBeDroppedFurther;
}

bool peano4::particles::particleWillBeDroppedFurther(
  const double                        searchRadius,
  const peano4::dataview::CellMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined())
    and peano4::particles::fitsIntoLevel(searchRadius * MeshWidthRatio, marker);
}

bool peano4::particles::particleWillBeDroppedFurther(
  const double                          searchRadius,
  const peano4::dataview::VertexMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined())
    and peano4::particles::fitsIntoLevel(searchRadius * MeshWidthRatio, marker);
}
