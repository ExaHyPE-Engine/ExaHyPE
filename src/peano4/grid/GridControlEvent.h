// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridControlEvent;
} // namespace peano4::grid

/**
 * A request to refine or erase, named by the cell that raised it.
 *
 * Cells sit on a lattice per level with peano4::sfc::ActiveCurve::SubdivisionFactor cells
 * per parent cell along each axis, so a level and a lattice position name a cell exactly
 * and the grid follows from integer arithmetic on them. Trivially copyable, so a request
 * travels between ranks as bytes.
 */
struct peano4::grid::GridControlEvent {
public:
  enum class RefinementControl : std::uint8_t { Refine = 0, Erase = 1 };

  std::uint8_t refinementControl = static_cast<std::uint8_t>(RefinementControl::Refine);

  /** Grid level of the cell that raised the request, counted from the root cell. */
  std::int32_t level = 0;

  /** Position of that cell on its level's lattice. */
  tarch::la::Vector<DIMENSIONS, int> latticeIndex{0};

  GridControlEvent() noexcept = default;

  [[nodiscard]] INLINE_METHOD RefinementControl getRefinementControl() const noexcept;
  INLINE_METHOD void                            setRefinementControl(RefinementControl value) noexcept;

  static GridControlEvent forCell(
    RefinementControl                            control,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellH,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );

  [[nodiscard]] std::string toString() const;

  bool operator<(const GridControlEvent& other) const noexcept;
  bool operator==(const GridControlEvent& other) const noexcept;
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridControlEvent& event);

#include "peano4/grid/GridControlEvent.cpph"
