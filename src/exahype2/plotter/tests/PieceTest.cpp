// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PieceTest.h"

#include "exahype2/plotter/Piece.h"
#include "exahype2/plotter/tests/RecordingFileWriter.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  int plotUnitCell(exahype2::plotter::Piece& piece) {
    std::array<int, peano4::VerticesPerCell> vertices{};
    for (int corner = 0; corner < peano4::VerticesPerCell; corner++) {
      tarch::la::Vector<DIMENSIONS, double> position;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        position(axis) = static_cast<double>(exahype2::plotter::CellCorners[corner][axis]);
      }
      vertices[corner] = piece.plotVertex(position);
    }
    return piece.plotCell(vertices);
  }
} // namespace

exahype2::plotter::tests::PieceTest::PieceTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::PieceTest") {}

void exahype2::plotter::tests::PieceTest::run() {
  testMethod(testWritesTheMarkersOfEveryCell);
  testMethod(testCreatesTheFieldsInTheOrderOfTheCalls);
  testMethod(testMergesTheVerticesOfOnePosition);
  testMethod(testKeepsTheVerticesOfOnePositionDistinct);
}

/**
 * Checks that every plotted cell carries the rank, the tree number of the piece and the level
 * the caller hands over.
 */
void exahype2::plotter::tests::PieceTest::testWritesTheMarkersOfEveryCell() {
  exahype2::plotter::Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  exahype2::plotter::Piece piece;
  piece.open(snapshot, 5, tarch::plotter::FileWriter::VertexMerging::Merged);
  validateTrue(piece.isOpen());

  const int cellIndex = plotUnitCell(piece);
  validateEquals(cellIndex, 0);
  piece.plotCellMarkers(cellIndex, 4);

  const auto& writer = getRecordingFileWriter(snapshot);
  validateEquals(writer.getNumberOfPoints(0), peano4::VerticesPerCell);
  validateEquals(writer.getNumberOfCells(0), 1);
  validateEquals(static_cast<int>(writer.getCellField(0, "Subdomain").size()), 1);
  validateNumericalEquals(
    writer.getCellField(0, "Subdomain")[0],
    static_cast<double>(tarch::mpi::Rank::getInstance().getRank())
  );
  validateNumericalEquals(writer.getCellField(0, "Subtree")[0], 5.0);
  validateNumericalEquals(writer.getCellField(0, "RefinementLevel")[0], 4.0);
}

/**
 * Checks that a piece writes the fields every piece carries first and the fields of the caller
 * in the order of the creation calls.
 */
void exahype2::plotter::tests::PieceTest::testCreatesTheFieldsInTheOrderOfTheCalls() {
  exahype2::plotter::Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  exahype2::plotter::Piece piece;
  piece.open(snapshot, 0, tarch::plotter::FileWriter::VertexMerging::Merged);
  const auto firstPlotter  = piece.createCellDataPlotter("First", 1);
  const auto secondPlotter = piece.createCellDataPlotter("Second", 1);
  const auto thirdPlotter  = piece.createVertexDataPlotter("Third", 2);

  const std::vector<std::string> expectedCellFields{"Subdomain", "Subtree", "RefinementLevel", "First", "Second"};
  const std::vector<std::string> expectedPointFields{"Third"};
  validateTrue(getRecordingFileWriter(snapshot).getCellFieldNames(0) == expectedCellFields);
  validateTrue(getRecordingFileWriter(snapshot).getPointFieldNames(0) == expectedPointFields);
  validateTrue(firstPlotter != nullptr);
  validateTrue(secondPlotter != nullptr);
  validateTrue(thirdPlotter != nullptr);
}

/**
 * Checks that a merging piece plots one point for two calls that carry the same position.
 */
void exahype2::plotter::tests::PieceTest::testMergesTheVerticesOfOnePosition() {
  exahype2::plotter::Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  exahype2::plotter::Piece piece;
  piece.open(snapshot, 0, tarch::plotter::FileWriter::VertexMerging::Merged);

  const tarch::la::Vector<DIMENSIONS, double> position(0.75);
  validateEquals(piece.plotVertex(position), 0);
  validateEquals(piece.plotVertex(position), 0);
  validateEquals(getRecordingFileWriter(snapshot).getNumberOfPoints(0), 1);
  validateNumericalEquals(getRecordingFileWriter(snapshot).getPointCoordinate(0, 0, 0), 0.75);
}

/**
 * Checks that a distinct piece plots two points for two calls that carry the same position.
 */
void exahype2::plotter::tests::PieceTest::testKeepsTheVerticesOfOnePositionDistinct() {
  exahype2::plotter::Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  exahype2::plotter::Piece piece;
  piece.open(snapshot, 0, tarch::plotter::FileWriter::VertexMerging::Distinct);

  const tarch::la::Vector<DIMENSIONS, double> position(0.75);
  validateEquals(piece.plotVertex(position), 0);
  validateEquals(piece.plotVertex(position), 1);
  validateEquals(getRecordingFileWriter(snapshot).getNumberOfPoints(0), 2);
}
