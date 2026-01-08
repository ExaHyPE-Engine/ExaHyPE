// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

tarch::plotter::FileWriter::CellDataPlotter::
  CellDataPlotter(FileWriter& plotter, int subdomainId, const std::string& id, int records):
  _impl(plotter, subdomainId, plotter._subdomains.at(subdomainId).cellFields, id, records) {}

void tarch::plotter::FileWriter::CellDataPlotter::plotCellData(int index, double value) {
  plotCellData<double>(index, value);
}

void tarch::plotter::FileWriter::CellDataPlotter::plotCellData(int index, const tarch::la::Vector<2, double>& value) {
  plotCellData<double>(index, value);
}

void tarch::plotter::FileWriter::CellDataPlotter::plotCellData(int index, const tarch::la::Vector<3, double>& value) {
  plotCellData<double>(index, value);
}

void tarch::plotter::FileWriter::CellDataPlotter::
  plotCellData(int index, const double* const values, int numberOfValues) {
  plotCellData<double>(index, values, numberOfValues);
}
