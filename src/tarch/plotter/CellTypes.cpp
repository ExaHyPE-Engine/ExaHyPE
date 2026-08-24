// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellTypes.h"

namespace {
  using LookupCellTypeByCodeFunction   = const tarch::plotter::CellTypeInfo* (*)(int vtkCode);
  using LookupCellTypeByTypeFunction   = const tarch::plotter::CellTypeInfo* (*)(tarch::plotter::CellType cellType);
  using NodesOfFunction                = int (*)(tarch::plotter::CellType cellType);
  using XDMFTypeNeedsNodeCountFunction = bool (*)(int xdmfType);

  [[maybe_unused]] constexpr LookupCellTypeByCodeFunction   LookupCellTypeByCode = &tarch::plotter::lookupCellType;
  [[maybe_unused]] constexpr LookupCellTypeByTypeFunction   LookupCellTypeByType = &tarch::plotter::lookupCellType;
  [[maybe_unused]] constexpr NodesOfFunction                NodesOf              = &tarch::plotter::nodesOf;
  [[maybe_unused]] constexpr XDMFTypeNeedsNodeCountFunction XDMFTypeNeedsNodeCount =
    &tarch::plotter::xdmfTypeNeedsNodeCount;
} // namespace
