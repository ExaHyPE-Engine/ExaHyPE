// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4 {
  template <int Base, int Exponent>
  constexpr int Power{Base * Power<Base, Exponent - 1>};

  template <int Base>
  constexpr int Power<Base, 0>{1};

  constexpr int FacesPerCell{2 * DIMENSIONS};

  constexpr int VerticesPerCell{Power<2, DIMENSIONS>};

  constexpr int CellsPerVertex{VerticesPerCell};

  /**
   * Width of the neighbourhood of a cell along one axis: The cell itself and
   * its neighbour on either side.
   */
  constexpr int NeighbourhoodWidth{3};

  /**
   * Number of cells in the neighbourhood of a cell: The 3^d block of cells
   * that touch at least one vertex of it, the cell included.
   */
  constexpr int CellsPerNeighbourhood{Power<NeighbourhoodWidth, DIMENSIONS>};
} // namespace peano4
