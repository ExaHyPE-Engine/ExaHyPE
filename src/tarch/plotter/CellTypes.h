// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstdint>

namespace tarch::plotter {
  /**
   * The cell types the plotter understands, gathered in one table.
   *
   * The numeric values are the standard VTK cell-type codes (see the VTK file
   * format specification). Every plotter backend (VTU, HDF5/XDMF, NetCDF/UGRID)
   * as well as the CellPlotter front-end must agree on these codes, the number
   * of corner nodes, the topological dimension and the human-readable name.
   */
  enum class CellType : std::uint8_t {
    Vertex      = 1,
    Line        = 3,
    Triangle    = 5,
    Quadrangle  = 9,
    Tetrahedron = 10,
    Hexahedron  = 12,
    Wedge       = 13,
    Pyramid     = 14
  };

  struct CellTypeInfo {
    CellType    type;      //< VTK cell-type code
    int         nodes;     //< number of corner nodes
    int         dimension; //< topological dimension (0=point, 1=line, 2=face, 3=volume)
    const char* name;      //< human/UGRID name
    int         xdmf;      //< XDMF topology type code (0 == not representable in XDMF)
  };

  /**
   * The cell-type table, ordered by VTK code. Add a row here (and nowhere else)
   * to teach the whole plotter about a new cell type.
   */
  inline constexpr CellTypeInfo CellTypes[] = {
    {CellType::Vertex, 1, 0, "vertex", 1},
    {CellType::Line, 2, 1, "line", 2},
    {CellType::Triangle, 3, 2, "triangle", 4},
    {CellType::Quadrangle, 4, 2, "quadrangle", 5},
    {CellType::Tetrahedron, 4, 3, "tetrahedron", 6},
    {CellType::Hexahedron, 8, 3, "hexahedron", 9},
    {CellType::Wedge, 6, 3, "wedge", 8},
    {CellType::Pyramid, 5, 3, "pyramid", 7}
  };

  /**
   * Look a cell type up by its raw VTK code. Returns nullptr for an unknown
   * code so callers can decide whether that is an error.
   */
  constexpr const CellTypeInfo* lookupCellType(int vtkCode) {
    for (const CellTypeInfo& info : CellTypes) {
      if (static_cast<int>(info.type) == vtkCode) {
        return &info;
      }
    }
    return nullptr;
  }

  constexpr const CellTypeInfo* lookupCellType(CellType type) { return lookupCellType(static_cast<int>(type)); }

  /**
   * Number of corner nodes of a (known) cell type, usable in constant
   * expressions. Returns -1 for an unknown type.
   */
  constexpr int nodesOf(CellType type) {
    const CellTypeInfo* info = lookupCellType(type);
    return info != nullptr ? info->nodes : -1;
  }

  /**
   * The XDMF "poly" topologies (Polyvertex=1, Polyline=2, Polygon=3) are
   * variable-length and therefore require an explicit node-count integer in
   * the mixed-topology stream; all other XDMF topologies have a fixed arity.
   */
  constexpr bool xdmfTypeNeedsNodeCount(int xdmfType) { return xdmfType >= 1 and xdmfType <= 3; }
} // namespace tarch::plotter
