// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::plotter {
  enum class CellType : std::uint8_t;
} // namespace tarch::plotter

/**
 * Lists the VTK codes of the cell types that the plotters write.
 */
enum class tarch::plotter::CellType : std::uint8_t { Vertex = 1, Line = 3, Quadrangle = 9, Hexahedron = 12 };
