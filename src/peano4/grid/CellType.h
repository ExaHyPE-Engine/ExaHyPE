// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "VertexType.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  enum class CellType : std::uint8_t { New, Persistent, Delete };

  std::string toString(CellType type);

  /**
   * You pass in the vertices and it gives you back the cell type.
   * This routine translates the 2^d vertices of a cell into a cell type.
   *
   * @see getFaceType()
   */
  CellType getCellType(const VertexType vertexTypes[TWO_POWER_D]);
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& out, const peano4::grid::CellType& type);
