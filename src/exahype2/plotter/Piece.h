// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Snapshot.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"

namespace exahype2::plotter {
  class Piece;
  struct DataField;

  /**
   * Corner positions of a plotted quadrangle and hexahedron in the order the plot file expects.
   */
  constexpr std::array<std::array<int, 3>, 8> CellCorners{
    {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}
  };
} // namespace exahype2::plotter

struct exahype2::plotter::DataField final {
  std::string name{};
  int         components{1};
};

/**
 * Owns the plotters of one piece of a snapshot file. One tree acquires one piece and writes its
 * vertices, its cells, and the rank, the tree and the level of every cell into that piece.
 */
class exahype2::plotter::Piece final {
public:
  Piece()                        = default;
  Piece(const Piece&)            = delete;
  Piece(Piece&&)                 = delete;
  Piece& operator=(const Piece&) = delete;
  Piece& operator=(Piece&&)      = delete;
  ~Piece()                       = default;

  void open(Snapshot& snapshot, int treeId, tarch::plotter::FileWriter::VertexMerging vertexMerging);

  [[nodiscard]] std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> createCellDataPlotter(
    const std::string& name,
    int                components
  );

  [[nodiscard]] std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> createVertexDataPlotter(
    const std::string& name,
    int                components
  );

  [[nodiscard]] INLINE_METHOD bool isOpen() const;

  INLINE_METHOD int plotVertex(const tarch::la::Vector<DIMENSIONS, double>& position);

  INLINE_METHOD int plotPoint(int vertexIndex);

  INLINE_METHOD int plotCell(const std::array<int, peano4::VerticesPerCell>& vertexIndices);

  INLINE_METHOD int plotCellOnFace(const std::array<int, peano4::VerticesPerCell / 2>& vertexIndices);

  INLINE_METHOD void plotCellMarkers(int cellIndex, int level);

private:
  int _treeId{-1};
  int _pieceId{-1};

  tarch::plotter::FileWriter* _fileWriter{nullptr};

  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter>   _vertexPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter>     _cellPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subdomainPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subtreeTaskPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _refinementLevelPlotter{};
};

#include "exahype2/plotter/Piece.cpph"
