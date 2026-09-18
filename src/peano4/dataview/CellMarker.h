// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::dataview {
  struct CellMarker;

  std::ostream& operator<<(std::ostream& out, const CellMarker& marker);
} // namespace peano4::dataview

/**
 * Describes the cell a traversal event visits: Its level, width, centre and
 * position within the father cell, plus its refinement and ownership flags.
 * The traversal automaton builds one marker per cell.
 */
struct peano4::dataview::CellMarker final {
  int level{0};

  tarch::la::Vector<DIMENSIONS, double> h{};
  tarch::la::Vector<DIMENSIONS, double> centre{};

  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell{};

  enum : std::uint16_t {
    HasBeenRefined = 1U << 0,
    WillBeRefined  = 1U << 1,
    IsLocal        = 1U << 2,
    IsParentLocal  = 1U << 3
  };

  std::uint16_t flags{0};

  [[nodiscard]] INLINE_METHOD bool hasBeenRefined() const;
  [[nodiscard]] INLINE_METHOD bool willBeRefined() const;
  [[nodiscard]] INLINE_METHOD bool isLocal() const;
  [[nodiscard]] INLINE_METHOD bool isParentLocal() const;

  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const;
  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getOffset() const;
  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionWithinFatherCell() const;

  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const;

  [[nodiscard]] INLINE_METHOD bool isContained(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    double                                       tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;

  static INLINE_METHOD bool isContained(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       tolerance
  );

  [[nodiscard]] INLINE_METHOD bool overlaps(
    const tarch::la::Vector<DIMENSIONS, double>& regionOffset,
    const tarch::la::Vector<DIMENSIONS, double>& regionSize
  ) const;

  [[nodiscard]] std::string toString() const;
};

#include "peano4/dataview/CellMarker.cpph"
