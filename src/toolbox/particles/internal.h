// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "tarch/la/Vector.h"

namespace toolbox {
  namespace particles {
    /**
     * Sorting tolerance
     *
     * The particle association is neither unique nor sharp, as we work with
     * floating point numbers. The correlation
     *
     * @f$ centre_{left} + h/2 < centre_{right} - h/2 @f$
     *
     * might not hold for two neighbouring voxels, i.e. there might be a floating-point
     * gap in-between two voxels. If particles fall into this gap, they would
     * not belong to any voxel. We therefore give each voxel a small overlap.
     * The relative size of this overlap is quantified by ReleaseOwnershipSpatialSortingTolerance.
     * This is our first principle: particles are always assigned to a cell or
     * volume relative to a relative threshold. We issue a resort only when the
     * particle is outside of its respective control volume subject to the
     * additional tolerance. If a particle leaves a cell or vertex environment
     * subject to the ReleaseOwnershipSpatialSortingTolerance, we either assign it
     * to a neighbouring voxel or lift it into a coarser level or assign it to
     * a global sorting list.
     *
     * There is a second threshold: Our sorting should be idempotent. That is,
     * once we assign a particle to a vertex or cell, we should not immediately
     * require yet another resorting step. Two rules formalise this:
     *
     * 1. If a vertex releases a stationary particle, it will never grab it
     *    back.
     * 2. If a vertex grabs a stationary particle, it will never release it.
     *
     * The two principles imply that we work with two thresholds: We trigger a
     * resort if a particle leaves its control volume. The region in which we
     * grab a particle is slightly smaller, so releases are more conservative
     * than grabs or drops.
     *
     * @image html MultiscaleTransitions.png
     *
     * ## Related action sets
     *
     * The action set peano4.toolbox.particles.api.UpdateParallelState employs the
     * threshold to decide if a particle is local or not. It has to use the
     * most generic environment, i.e. the release radius.
     *
     * @see GrabOwnershipSpatialSortingTolerance
     */
    constexpr double ReleaseOwnershipSpatialSortingTolerance = 1.01;

    /**
     * @see ReleaseOwnershipSpatialSortingTolerance
     */
    constexpr double GrabOwnershipSpatialSortingTolerance = 1.001;

    namespace internal {
      double relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker);
      double relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker);
      double relativeReleaseOwnershipSpatialSortingTolerance(const tarch::la::Vector<DIMENSIONS, double>& h);
      double relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker);
      double relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker);
      double relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector<DIMENSIONS, double>& h);

      bool fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::CellMarker& marker);
      bool fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::VertexMarker& marker);

      /**
       * Find out which vertex should hold a particle.
       */
      std::bitset<DIMENSIONS> getParticleAssociationWithinCell(
        const tarch::la::Vector<DIMENSIONS, double>& x,
        const tarch::la::Vector<DIMENSIONS, double>& cellCentre
      );
    } // namespace internal
  } // namespace particles
} // namespace toolbox
