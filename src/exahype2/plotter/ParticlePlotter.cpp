// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticlePlotter.h"

/**
 * Claims the piece of one tree and creates one cell-field plotter per attribute.
 */
void exahype2::plotter::ParticlePlotter::open(
  Snapshot&                     snapshot,
  const int                     treeId,
  const std::vector<DataField>& attributes
) {
  _piece.open(snapshot, treeId, tarch::plotter::FileWriter::VertexMerging::Merged);

  _attributePlotters.clear();
  for (const auto& attribute : attributes) {
    _attributePlotters.push_back(_piece.createCellDataPlotter(attribute.name, attribute.components));
  }
}
