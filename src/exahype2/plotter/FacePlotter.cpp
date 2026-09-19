// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FacePlotter.h"

/**
 * Claims the piece of one tree and creates two node-field plotters per field. Each field starts
 * where the fields before it end.
 */
void exahype2::plotter::FacePlotter::open(Snapshot& snapshot, const int treeId, const std::vector<DataField>& fields) {
  _piece.open(snapshot, treeId, tarch::plotter::FileWriter::VertexMerging::Distinct);

  _fields.clear();
  int offset = 0;
  for (const auto& field : fields) {
    _fields.push_back(
      {_piece.createVertexDataPlotter(field.name + "L", field.components),
       _piece.createVertexDataPlotter(field.name + "R", field.components),
       field.components,
       offset}
    );
    offset += field.components;
  }
}
