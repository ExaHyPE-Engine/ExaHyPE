// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridVertex;

  enum class VertexType : std::uint8_t { New, Hanging, Persistent, Delete };

  std::string toString(VertexType type);

  void getVertexTypes(
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& position,
    VertexType                                result[TWO_POWER_D]
  );
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& out, const peano4::grid::VertexType& type);
