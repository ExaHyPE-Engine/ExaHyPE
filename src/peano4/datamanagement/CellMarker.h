// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::datamanagement {
  struct CellMarker;
} // namespace peano4::datamanagement

/**
 * Cell marker
 *
 * This object provides information on the cell aka octant within the
 * spacetree. This includes spatial
 * information such as size and position. It also comprises mesh topology
 * properties such as "will this cell be refined" or "is its parent a
 * local cell".
 *
 * It is an object which users are not supposed to manipulate. The mesh
 * traversal automaton creates it once per octant while it runs through
 * the mesh and passes in information about adjacent vertices, parent
 * data, and so forth. The constructor then distills the user
 * representation from that data and, from hereon, users can use this
 * distilled information to make logical decisions what to do and how
 * to do things within the spacetree octant.
 */
struct peano4::datamanagement::CellMarker {
  tarch::la::Vector<DIMENSIONS, double> h{};
  tarch::la::Vector<DIMENSIONS, double> centre{};

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell{};

  enum : std::uint16_t {
    HasBeenRefined               = 1u << 0,
    WillBeRefined                = 1u << 1,
    IsLocal                      = 1u << 2,
    IsParentLocal                = 1u << 3,
    AllVerticesRefined           = 1u << 4,
    OneVertexHanging             = 1u << 5,
    OneVertexCreatedOrDestroyed  = 1u << 6,
    AllVerticesInsideDomain      = 1u << 7,
    InvokingTreeNotInvolvedInDLB = 1u << 8,
    WillBeEnclave                = 1u << 9,
    HasBeenEnclave               = 1u << 10
  };

  std::uint16_t flags{0};

  INLINE_METHOD bool hasBeenRefined() const { return flags & HasBeenRefined; }
  INLINE_METHOD bool willBeRefined() const { return flags & WillBeRefined; }
  INLINE_METHOD bool isLocal() const { return flags & IsLocal; }
  INLINE_METHOD bool isParentLocal() const { return flags & IsParentLocal; }
  INLINE_METHOD bool hasBeenEnclaveCell() const { return flags & HasBeenEnclave; }
  INLINE_METHOD bool willBeEnclaveCell() const { return flags & WillBeEnclave; }
  INLINE_METHOD bool hasBeenSkeletonCell() const { return not(flags & HasBeenEnclave); }
  INLINE_METHOD bool willBeSkeletonCell() const { return not(flags & WillBeEnclave); }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const { return h; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const { return centre; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double> getOffset() const { return centre - 0.5 * h; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionWithinFatherCell() const {
    return relativePositionWithinFatherCell;
  }

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const {
    tarch::la::Vector<DIMENSIONS, double> result = centre;
    for (int d = 0; d < DIMENSIONS; d++) {
      assertion1(relativePositionWithinFatherCell[d] >= 0, toString());
      assertion1(relativePositionWithinFatherCell[d] <= 2, toString());
      if (relativePositionWithinFatherCell[d] == 0) {
        result[d] += h[d];
      }
      if (relativePositionWithinFatherCell[d] == 2) {
        result[d] -= h[d];
      }
    }
    return result;
  }

  INLINE_METHOD bool isContained(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const {
    return isContained(x, centre, h, tolerance);
  }

  static INLINE_METHOD bool isContained(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       tolerance
  ) {
    bool result = true;
    for (int d = 0; d < DIMENSIONS; d++) {
      result &= tarch::la::greaterEquals(x(d), cellCentre(d) - h(d) / 2.0, tolerance);
      result &= tarch::la::smallerEquals(x(d), cellCentre(d) + h(d) / 2.0, tolerance);
    }
    return result;
  }

  INLINE_METHOD bool overlaps(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& size
  ) const {
    bool overlaps = true;

    for (int d = 0; d < DIMENSIONS; d++) {
      overlaps &= tarch::la::smallerEquals(centre(d) - h(d) / 2.0, offset(d) + size(d));
      overlaps &= tarch::la::greaterEquals(centre(d) + h(d) / 2.0, offset(d));
    }

    return overlaps;
  }

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::CellMarker& marker);
