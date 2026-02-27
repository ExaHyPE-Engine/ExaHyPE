// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
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

  /**
   * Reconstruct the fine-grid VertexType array as it would have been in the previous
   * tree sweep.
   *
   * We do not persist VertexType itself. Instead, we store one bit per coarse vertex
   * (GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep) that tells us whether
   * the vertex mapped to VertexType::Hanging in the previous sweep. All other coarse
   * vertices are treated as VertexType::Persistent.
   */
  void getVertexTypesFromPreviousTreeSweep(
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& position,
    VertexType                                result[TWO_POWER_D]
  );
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& out, const peano4::grid::VertexType& type);
