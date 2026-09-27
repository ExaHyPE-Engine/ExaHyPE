// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/FileWriter.h"

tarch::plotter::FileWriter::VertexDataPlotter::VertexDataPlotter(
  FileWriter&        plotter,
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerVertex
) :
  _impl(plotter.getPieceBuffers(subdomainId).pointFields, identifier, recordsPerVertex) {}

void
tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(const int index, const double value) {
  _impl.writeTuple(index, &value, 1);
}

void
tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int                           index,
  const tarch::la::Vector<2, double>& value
) {
  _impl.writeTuple(index, value.getData(), 2);
}

void
tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int                           index,
  const tarch::la::Vector<3, double>& value
) {
  _impl.writeTuple(index, value.getData(), 3);
}

void
tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int           index,
  const double* const values,
  const int           numberOfValues
) {
  _impl.writeTuple(index, values, numberOfValues);
}
