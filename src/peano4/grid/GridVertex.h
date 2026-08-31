// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
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

  INLINE_METHOD State getState() const noexcept { return static_cast<State>(state); }
  INLINE_METHOD void  setState(State value) noexcept { state = static_cast<std::uint8_t>(value); }

#if PEANO_DEBUG
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept { return x; }
  INLINE_METHOD void   setX(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept { x = value; }
  INLINE_METHOD double getX(int axis) const noexcept { return x[axis]; }
  INLINE_METHOD void   setX(int axis, double value) noexcept { x[axis] = value; }
#endif

  std::string toString() const;

  static bool areAllVerticesRefined(const GridVertex vertices[TWO_POWER_D]);

  static std::bitset<TWO_POWER_D> getRefinedVertices(const GridVertex vertices[TWO_POWER_D]);
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex);
