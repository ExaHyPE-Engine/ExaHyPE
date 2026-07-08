// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  struct GridControlEvent;
} // namespace peano4::grid

struct peano4::grid::GridControlEvent {
public:
  enum class RefinementControl : std::uint8_t { Refine = 0, Erase = 1 };

  std::uint8_t refinementControl = static_cast<std::uint8_t>(RefinementControl::Refine);

  tarch::la::Vector<DIMENSIONS, double> offset{};
  tarch::la::Vector<DIMENSIONS, double> width{};
  tarch::la::Vector<DIMENSIONS, double> targetH{};

  GridControlEvent() noexcept = default;

  INLINE_METHOD RefinementControl getRefinementControl() const noexcept {
    return static_cast<RefinementControl>(refinementControl);
  }
  INLINE_METHOD void setRefinementControl(RefinementControl rc) noexcept {
    refinementControl = static_cast<std::uint8_t>(rc);
  }

  std::string toString() const;

  bool operator<(const GridControlEvent& other) const noexcept;

  /**
   * Coalesce refinement/erase instructions into as few axis-aligned boxes as possible without
   * changing which cells they cover. The merge is exact and
   * tolerance-free; refine and erase events are coalesced independently.
   */
  static void merge(std::vector<GridControlEvent>& events);
};

std::ostream& operator<<(std::ostream& os, const peano4::grid::GridControlEvent& event);
