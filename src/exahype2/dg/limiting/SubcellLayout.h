// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"

namespace exahype2::dg::limiting {
  template <typename DGSolverType>
  struct SubcellLayout;

  template <int NumberOfSubcellsPerAxis, int NumberOfData>
  constexpr int ValuesPerSubcellPatch = NumberOfData * peano4::power(NumberOfSubcellsPerAxis, DIMENSIONS);
} // namespace exahype2::dg::limiting

/**
 * Derives the node and subcell counts of the subcell limiter from the order of a solver. The
 * subgrid of a cell carries 2 * order + 1 subcells per axis.
 */
template <typename DGSolverType>
struct exahype2::dg::limiting::SubcellLayout final {
  static constexpr int NumberOfDofsPerAxis     = DGSolverType::DGOrder + 1;
  static constexpr int NumberOfSubcellsPerAxis = 2 * DGSolverType::DGOrder + 1;
  static constexpr int NumberOfData     = DGSolverType::NumberOfUnknowns + DGSolverType::NumberOfAuxiliaryVariables;
  static constexpr int NumberOfNodes    = peano4::power(NumberOfDofsPerAxis, DIMENSIONS);
  static constexpr int NumberOfSubcells = peano4::power(NumberOfSubcellsPerAxis, DIMENSIONS);
};
