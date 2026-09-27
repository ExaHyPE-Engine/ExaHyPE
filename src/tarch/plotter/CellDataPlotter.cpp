// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/FileWriter.h"

tarch::plotter::FileWriter::CellDataPlotter::CellDataPlotter(
  FileWriter&        plotter,
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerCell
) :
  _impl(plotter.getPieceBuffers(subdomainId).cellFields, identifier, recordsPerCell) {}

void
tarch::plotter::FileWriter::CellDataPlotter::plotCellData(const int index, const double value) {
  plotCellData<double>(index, value);
}

void
tarch::plotter::FileWriter::CellDataPlotter::plotCellData(const int index, const tarch::la::Vector<2, double>& value) {
  plotCellData<double>(index, value);
}

void
tarch::plotter::FileWriter::CellDataPlotter::plotCellData(const int index, const tarch::la::Vector<3, double>& value) {
  plotCellData<double>(index, value);
}

void
tarch::plotter::FileWriter::CellDataPlotter::plotCellData(
  const int           index,
  const double* const values,
  const int           numberOfValues
) {
  plotCellData<double>(index, values, numberOfValues);
}
