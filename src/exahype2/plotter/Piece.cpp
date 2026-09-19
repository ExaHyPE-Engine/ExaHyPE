// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Piece.h"

/**
 * Claims the piece of one tree in the file of the snapshot and creates the plotters that every
 * piece writes. The tree number orders the pieces within the file, whatever order concurrent
 * traversals reach this point in.
 */
void exahype2::plotter::Piece::open(
  Snapshot&                                       snapshot,
  const int                                       treeId,
  const tarch::plotter::FileWriter::VertexMerging vertexMerging
) {
  assertion(treeId >= 0, treeId);

  _fileWriter = &snapshot.getFileWriter();
  _treeId     = treeId;
  _pieceId    = _fileWriter->acquirePiece(treeId);

  _subdomainPlotter       = _fileWriter->createCellDataPlotter(_pieceId, "Subdomain", 1);
  _subtreeTaskPlotter     = _fileWriter->createCellDataPlotter(_pieceId, "Subtree", 1);
  _refinementLevelPlotter = _fileWriter->createCellDataPlotter(_pieceId, "RefinementLevel", 1);

  _vertexPlotter = _fileWriter->createVertexPlotter(_pieceId, vertexMerging);
  _cellPlotter   = _fileWriter->createCellPlotter(_pieceId);
}

/**
 * The order of the creation calls is the order of the fields in the file.
 */
std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> exahype2::plotter::Piece::createCellDataPlotter(
  const std::string& name,
  const int          components
) {
  assertion(isOpen());
  assertion(components > 0, components);
  return _fileWriter->createCellDataPlotter(_pieceId, name, components);
}

/**
 * The order of the creation calls is the order of the fields in the file.
 */
std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> exahype2::plotter::Piece::createVertexDataPlotter(
  const std::string& name,
  const int          components
) {
  assertion(isOpen());
  assertion(components > 0, components);
  return _fileWriter->createVertexDataPlotter(_pieceId, name, components);
}
