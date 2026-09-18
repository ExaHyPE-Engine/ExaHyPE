// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridVertex;
} // namespace peano4::grid

struct peano4::grid::GridVertex {
  /**
   * A vertex is refined where the tree continues below it, hanging where a
   * refinement transition cuts through it, and unrefined otherwise. The
   * spacetree holds a settled structure, so a structure change is expressed by
   * the transition sweep's own before and after masks.
   */
  enum class State : std::uint8_t { HangingVertex = 0, Unrefined = 1, Refined = 2 };

  std::uint8_t state{static_cast<std::uint8_t>(State::Unrefined)};

  int level{0};

#if PEANO_DEBUG
  tarch::la::Vector<DIMENSIONS, double> x{};
#endif

  GridVertex() noexcept = default;

  [[nodiscard]] INLINE_METHOD State getState() const noexcept;
  INLINE_METHOD void                setState(State value) noexcept;

#if PEANO_DEBUG
  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept;
  INLINE_METHOD void                 setX(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept;
  [[nodiscard]] INLINE_METHOD double getX(int axis) const noexcept;
  INLINE_METHOD void                 setX(int axis, double value) noexcept;
#endif

  [[nodiscard]] std::string toString() const;

  static bool areAllVerticesRefined(const GridVertex vertices[peano4::VerticesPerCell]);

  static std::bitset<peano4::VerticesPerCell> getRefinedVertices(const GridVertex vertices[peano4::VerticesPerCell]);
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex);

#include "peano4/grid/GridVertex.cpph"
