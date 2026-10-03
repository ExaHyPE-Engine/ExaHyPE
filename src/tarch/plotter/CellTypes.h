// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::plotter {
  enum class CellType : std::uint8_t;
  struct CellTypeInfo;
} // namespace tarch::plotter

/**
 * Cell types of the plotters, numbered by their VTK cell-type codes.
 */
enum class tarch::plotter::CellType : std::uint8_t {
  Vertex      = 1,
  Line        = 3,
  Triangle    = 5,
  Quadrangle  = 9,
  Tetrahedron = 10,
  Hexahedron  = 12,
  Wedge       = 13,
  Pyramid     = 14
};

struct tarch::plotter::CellTypeInfo {
  tarch::plotter::CellType type;      ///< VTK cell-type code.
  int                      nodes;     ///< Number of corner nodes.
  int                      dimension; ///< Topological dimension from zero to three.
  const char*              name;      ///< Human-readable UGRID name.
  int                      xdmf;      ///< XDMF topology code, or zero if no representation exists.
};

namespace tarch::plotter {
  /**
   * Canonical cell-type mapping for all plotter backends, ordered by VTK code.
   */
  inline constexpr tarch::plotter::CellTypeInfo CellTypes[] = {
    {tarch::plotter::CellType::Vertex, 1, 0, "vertex", 1},
    {tarch::plotter::CellType::Line, 2, 1, "line", 2},
    {tarch::plotter::CellType::Triangle, 3, 2, "triangle", 4},
    {tarch::plotter::CellType::Quadrangle, 4, 2, "quadrangle", 5},
    {tarch::plotter::CellType::Tetrahedron, 4, 3, "tetrahedron", 6},
    {tarch::plotter::CellType::Hexahedron, 8, 3, "hexahedron", 9},
    {tarch::plotter::CellType::Wedge, 6, 3, "wedge", 8},
    {tarch::plotter::CellType::Pyramid, 5, 3, "pyramid", 7}
  };

  INLINE_METHOD constexpr const tarch::plotter::CellTypeInfo* lookupCellType(const int vtkCode);
  INLINE_METHOD constexpr const tarch::plotter::CellTypeInfo* lookupCellType(const tarch::plotter::CellType type);
  INLINE_METHOD constexpr int getNumberOfNodesOf(const tarch::plotter::CellType type);
  INLINE_METHOD constexpr bool xdmfTypeNeedsNodeCount(const int xdmfType);
} // namespace tarch::plotter

#include "tarch/plotter/CellTypes.cpph"
