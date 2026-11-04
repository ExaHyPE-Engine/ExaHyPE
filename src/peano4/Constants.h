// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace peano4 {
  /**
   * Returns base raised to the non-negative exponent.
   */
  constexpr int power(const int base, const int exponent) {
    int result = 1;
    for (int multiplication = 0; multiplication < exponent; multiplication++) {
      result *= base;
    }
    return result;
  }

  constexpr int FacesPerCell{2 * DIMENSIONS};

  constexpr int VerticesPerCell{peano4::power(2, DIMENSIONS)};

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
  constexpr int CellsPerNeighbourhood{peano4::power(NeighbourhoodWidth, DIMENSIONS)};
} // namespace peano4
TARGET_DECLARE_END
