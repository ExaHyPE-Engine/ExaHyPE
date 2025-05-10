// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeanoHDF5FileWriter.h"

#ifdef UseHDF5

#include "tarch/la/ScalarOperations.h"

#include <iomanip>

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::VertexDataWriter(
  const std::string&                                              identifier,
  int                                                             unknownsPerAxis,
  int                                                             numberOfUnknowns,
  const std::string&                                              description,
  const std::string&                                              metaData,
  double*                                                         mapping,
  tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& writer
):
  _writer(writer),
  _identifier(identifier),
  _numberOfVerticesPerAxis(unknownsPerAxis),
  _numberOfUnknowns(numberOfUnknowns),
  _entryCounter(0) {}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::~VertexDataWriter() {}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::setPrecision(int precision) {}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::getVerticesPerPatch() const {
  return tarch::la::aPowI(_writer._dimensions, _numberOfVerticesPerAxis);
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::plotVertex(
  [[maybe_unused]] int index,
  double               value
) {}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::plotVertex(
  [[maybe_unused]] int index,
  double*              values
) {}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::close() {}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::
  assignRemainingVerticesDefaultValues() {
  assertionMsg(false, "not yet implemented");
}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter::getFirstVertexWithinPatch(
  int index
) const {
  return index * getVerticesPerPatch();
}

#endif
