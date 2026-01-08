// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

tarch::plotter::FileWriter::VertexDataPlotter::
  VertexDataPlotter(FileWriter& plotter, int subdomainId, const std::string& id, int records):
  _impl(plotter, subdomainId, plotter._subdomains.at(subdomainId).pointFields, id, records) {}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(int index, double value) {
  _impl.writeTuple(index, &value, 1);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  int                                 index,
  const tarch::la::Vector<2, double>& value
) {
  _impl.writeTuple(index, value.data(), 2);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  int                                 index,
  const tarch::la::Vector<3, double>& value
) {
  _impl.writeTuple(index, value.data(), 3);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(int index, double* values, int numberOfValues) {
  _impl.writeTuple(index, values, numberOfValues);
}
