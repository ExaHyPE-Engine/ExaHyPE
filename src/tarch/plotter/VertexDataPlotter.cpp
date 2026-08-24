// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

tarch::plotter::FileWriter::VertexDataPlotter::VertexDataPlotter(
  FileWriter&        plotter,
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerVertex
) :
  _impl(
    plotter,
    subdomainId,
    plotter._subdomains.at(static_cast<std::size_t>(subdomainId)).pointFields,
    identifier,
    recordsPerVertex
  ) {}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(const int index, const double value) {
  _impl.writeTuple(index, &value, 1);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int                           index,
  const tarch::la::Vector<2, double>& value
) {
  _impl.writeTuple(index, value.data(), 2);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int                           index,
  const tarch::la::Vector<3, double>& value
) {
  _impl.writeTuple(index, value.data(), 3);
}

void tarch::plotter::FileWriter::VertexDataPlotter::plotVertexData(
  const int           index,
  const double* const values,
  const int           numberOfValues
) {
  _impl.writeTuple(index, values, numberOfValues);
}
