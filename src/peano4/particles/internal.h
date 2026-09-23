// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"
#include "tarch/la/Vector.h"

namespace peano4::particles {
  /**
   * Relative overlap of two neighbouring voxels at which a vertex releases a
   * particle. Floating-point arithmetic leaves a gap between two voxels, and
   * the overlap holds a particle in that gap in both voxels. The release
   * overlap is larger than the grab overlap, so a vertex that releases a
   * particle at rest does not grab it back.
   *
   * @image html MultiscaleTransitions.png
   * @see GrabOwnershipSpatialSortingTolerance
   */
  constexpr double ReleaseOwnershipSpatialSortingTolerance = 1.01;

  /**
   * Relative overlap of two neighbouring voxels at which a vertex grabs a
   * particle.
   *
   * @see ReleaseOwnershipSpatialSortingTolerance
   */
  constexpr double GrabOwnershipSpatialSortingTolerance = 1.001;

  double relativeReleaseOwnershipSpatialSortingTolerance(const peano4::dataview::VertexMarker& marker);
  double relativeReleaseOwnershipSpatialSortingTolerance(const peano4::dataview::CellMarker& marker);
  double relativeReleaseOwnershipSpatialSortingTolerance(const tarch::la::Vector<DIMENSIONS, double>& h);
  double relativeGrabOwnershipSpatialSortingTolerance(const peano4::dataview::VertexMarker& marker);
  double relativeGrabOwnershipSpatialSortingTolerance(const peano4::dataview::CellMarker& marker);
  double relativeGrabOwnershipSpatialSortingTolerance(const tarch::la::Vector<DIMENSIONS, double>& h);

  bool fitsIntoLevel(double searchRadius, const peano4::dataview::CellMarker& marker);
  bool fitsIntoLevel(double searchRadius, const peano4::dataview::VertexMarker& marker);

  std::bitset<DIMENSIONS> getParticleAssociationWithinCell(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre
  );
} // namespace peano4::particles
