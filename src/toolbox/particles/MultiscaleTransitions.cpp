// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiscaleTransitions.h"

#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

/**
 * Ownership tolerance
 *
 * A particle is "owned" by a cell (or vertex, respectively), if it resides
 * within its area. We slightly augment this area, and this tolerance
 * reflects the augmentation. The result is
 * internal::relativeReleaseOwnershipSpatialSortingTolerance() multiplied
 * with the numerical zero tarch::la::NUMERICAL_ZERO_DIFFERENCE.
 */
double toolbox::particles::relativeSpatialOwnershipTolerance(const ::peano4::datamanagement::CellMarker& marker) {
  return internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
}

toolbox::particles::ParticleReassociationInstruction toolbox::particles::liftParticleAssociatedWithVertex(
  bool                                         isLocal,
  double                                       searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker   marker
) {
  const double tolerance =
    internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  if (not isLocal) {
    return ParticleReassociationInstruction_Keep;
  } else if (internal::fitsIntoLevel(searchRadius, marker) and marker.isContainedInAdjacentCells(x, 0.5, tolerance)) {
    return ParticleReassociationInstruction_Keep;
  } else if (marker.isParentCellLocal()
             and peano4::datamanagement::CellMarker::isContained(
               x,
               marker.getInvokingParentCellsCentre(),
               marker.getH() * 3.0,
               tolerance
             )) {
    std::bitset<DIMENSIONS> target =
      internal::getParticleAssociationWithinCell(x, marker.getInvokingParentCellsCentre());
    if (marker.isParentVertexLocal(target.to_ulong())
        and toolbox::particles::particleAssignedToVertexWillBeLocal(x, marker)) {
      return target.to_ulong();
    } else {
      return ParticleReassociationInstruction_SieveGlobally;
    }
  } else {
    return ParticleReassociationInstruction_SieveGlobally;
  }
}

toolbox::particles::ParticleReassociationInstruction
toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
  bool                                         isLocal,
  double                                       searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::CellMarker&    marker,
  int                                          numberOfVertexWithinCell
) {
  if (not isLocal) {
    return ParticleReassociationInstruction_Keep;
  } else {
    const std::bitset<DIMENSIONS> currentAssociation = numberOfVertexWithinCell;
    const std::bitset<DIMENSIONS> destAssociation    = internal::getParticleAssociationWithinCell(x, marker.getX());

    // marker.getX() gives me the centre of the cell
    bool remainsOnSameLevel = internal::fitsIntoLevel(searchRadius, marker);
    for (int d = 0; d < DIMENSIONS; d++) {
      remainsOnSameLevel &= tarch::la::smallerEquals(
        std::abs(x(d) - marker.getX()(d)),
        marker.getH()(d) * ReleaseOwnershipSpatialSortingTolerance
      );
    }

    if (remainsOnSameLevel and currentAssociation == destAssociation) {
      return ParticleReassociationInstruction_Keep;
    } else if (remainsOnSameLevel) {
      logDebug(
        "reassign particle at " << x << " within cell " << marker.toString()
                                << " which had been associated with vertex " << currentAssociation << " but belongs to "
                                << destAssociation
      );
      return destAssociation.to_ulong();
    } else if (not marker.isParentLocal()) {
      return ParticleReassociationInstruction_SieveGlobally;
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
 * Alternative version of dropParticle() without templates
 *
 * This is the actual implementation, i.e. the template delegates to this
 * one. Through the method, we can unittest the drop logic.
 */
bool toolbox::particles::dropParticle(
  double                                       searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  bool result = internal::fitsIntoLevel(searchRadius, marker)
    and marker.isContainedInAdjacentCells(
      x,
      0.5,
      internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    );

  assertion(
    not(
      result
      and liftParticleAssociatedWithVertex(true, searchRadius, x, marker)
        == ParticleReassociationInstruction_SieveGlobally
    ),
    searchRadius,
    x,
    marker.toString()
  );

  return result;
}

bool toolbox::particles::sieveParticle(
  double                                       searchRadius,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  bool belongsIntoNeighbouringCells = marker.isContainedInAdjacentCells(
    x,
    0.5,
    internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  bool fitsIntoCurrentMeshLevel = internal::fitsIntoLevel(searchRadius, marker);
  bool willNotBeDroppedFurther = not internal::fitsIntoLevel(searchRadius * 3.0, marker) or not marker.hasBeenRefined();

  return belongsIntoNeighbouringCells and fitsIntoCurrentMeshLevel and willNotBeDroppedFurther;
}

bool toolbox::particles::particleWillBeDroppedFurther(
  double                                    searchRadius,
  const peano4::datamanagement::CellMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}

bool toolbox::particles::particleWillBeDroppedFurther(
  double                                      searchRadius,
  const peano4::datamanagement::VertexMarker& marker
) {
  return (marker.hasBeenRefined() or marker.willBeRefined()) and internal::fitsIntoLevel(searchRadius * 3.0, marker);
}

/**
 * Find out which adjacent cell will hold a particle.
 *
 * Let the routine find out which of the neighbouring cells will try to
 * claim ownership of a particle. In exact arithmetics, this routine would
 * be the same as internal::getParticleAssociationWithinCell(). However, we
 * work with floating point precision and cells with a slight overlap.
 * Therefore, multiple adjacent cells might try to claim ownership of the
 * particle potentially, and this is important to find out a priori whether
 * a particle will be labelled as inside or not.
 *
 * For the floating point reason, i.e. the fact that the particle ownership
 * is not unique, VertexMarker::getRelativeAdjacentCell() is a more strict
 * version of this routine and cannot be used in some cases.
 *
 * @param x      Position of the particle of interest.
 * @param marker Marker of the vertex whose adjacent cells are studied.
 */
std::bitset<TWO_POWER_D> toolbox::particles::getAdjacentCellsOwningParticle(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const peano4::datamanagement::VertexMarker&  marker
) {
  std::bitset<TWO_POWER_D>                    result         = 0;
  const tarch::la::Vector<DIMENSIONS, double> selectedVertex = marker.getX();
  const tarch::la::Vector<DIMENSIONS, double> h              = marker.getH();
  const double                                tolerance =
    internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    const tarch::la::Vector<DIMENSIONS, int> adjacentCell(k);
    tarch::la::Vector<DIMENSIONS, double>    adjacentCellCentre(0.0);
    for (int d = 0; d < DIMENSIONS; d++) {
      adjacentCellCentre(d) = selectedVertex(d) + (static_cast<double>(adjacentCell(d)) - 0.5) * h(d);
    }
    result[kScalar] = peano4::datamanagement::CellMarker::isContained(x, adjacentCellCentre, h, tolerance);
  });

  return result;
}
