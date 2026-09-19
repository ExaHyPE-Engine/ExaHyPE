// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchPlotter.h"

std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> exahype2::plotter::NodalPatchPlotter::
  createCellDataPlotter(const std::string& name, const int components) {
  return _piece.createCellDataPlotter(name, components);
}

std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> exahype2::plotter::CellPatchPlotter::createCellDataPlotter(
  const std::string& name,
  const int          components
) {
  return _piece.createCellDataPlotter(name, components);
}
