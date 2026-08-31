// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::datamanagement {
  struct CellMarker;
} // namespace peano4::datamanagement

/**
 * Describes the cell a traversal event visits: Its level, width, centre and
 * position within the father cell, plus its refinement and ownership flags.
 * The traversal automaton builds one marker per cell.
 */
struct peano4::datamanagement::CellMarker {
  int level{0};

  tarch::la::Vector<DIMENSIONS, double> h{};
  tarch::la::Vector<DIMENSIONS, double> centre{};

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell{};

  enum : std::uint16_t {
    HasBeenRefined = 1U << 0,
    WillBeRefined  = 1U << 1,
    IsLocal        = 1U << 2,
    IsParentLocal  = 1U << 3
  };

  std::uint16_t flags{0};

  INLINE_METHOD bool hasBeenRefined() const { return (flags & HasBeenRefined) != 0; }
  INLINE_METHOD bool willBeRefined() const { return (flags & WillBeRefined) != 0; }
  INLINE_METHOD bool isLocal() const { return (flags & IsLocal) != 0; }
  INLINE_METHOD bool isParentLocal() const { return (flags & IsParentLocal) != 0; }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const { return h; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const { return centre; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double> getOffset() const { return centre - 0.5 * h; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionWithinFatherCell() const {
    return relativePositionWithinFatherCell;
  }

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const {
    tarch::la::Vector<DIMENSIONS, double> result = centre;
    for (int d = 0; d < DIMENSIONS; d++) {
      assertion(relativePositionWithinFatherCell[d] >= 0, toString());
      assertion(relativePositionWithinFatherCell[d] <= 2, toString());
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
