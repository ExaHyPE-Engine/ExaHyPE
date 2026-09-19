// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SnapshotTest.h"

#include "exahype2/plotter/Snapshot.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/plotter/VTUFileWriter.h"

namespace {
  std::string testDirectory() { return tarch::tests::getWritableTestDirectory().string(); }

  std::string snapshotFileName(const std::string& filename, const double time) {
    std::ostringstream stream;
    stream << filename << "-" << std::fixed << std::setprecision(6) << time;
    std::string name = stream.str();
    std::replace(name.begin(), name.end(), '.', '_');
    return name + "-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank()) + ".vtu";
  }

  std::string readFile(const std::filesystem::path& path) {
    const std::ifstream stream(path, std::ios::binary);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    return buffer.str();
  }

  void plotOnePoint(tarch::plotter::FileWriter& fileWriter) {
    const int  pieceId       = fileWriter.acquirePiece(0);
    const auto vertexPlotter = fileWriter.createVertexPlotter(pieceId);
    const auto cellPlotter   = fileWriter.createCellPlotter(pieceId);
    cellPlotter->plotPoint(vertexPlotter->plotVertex(tarch::la::Vector<DIMENSIONS, double>(0.5)));
  }

  void removeSnapshotFiles(const std::string& filename, const std::vector<double>& times) {
    for (const double time : times) {
      std::filesystem::remove(std::filesystem::path(testDirectory()) / snapshotFileName(filename, time));
    }
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      for (const double time : times) {
        const std::string name = snapshotFileName(filename, time);
        std::filesystem::remove(
          std::filesystem::path(testDirectory()) / (name.substr(0, name.find("-rank-")) + ".pvtu")
        );
      }
      std::filesystem::remove(std::filesystem::path(testDirectory()) / (filename + ".pvd"));
    }
  }
} // namespace

exahype2::plotter::tests::SnapshotTest::SnapshotTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::SnapshotTest") {}

void exahype2::plotter::tests::SnapshotTest::run() {
  testMethod(testWritesTheFileOfOneSnapshot);
  testMethod(testCollectsEverySnapshotInTheMetaFile);
}

/**
 * Checks that one snapshot writes the file of its traversal under the name and the time it
 * carries.
 */
void exahype2::plotter::tests::SnapshotTest::testWritesTheFileOfOneSnapshot() {
  const std::string filename = "ExaHyPE2PlotterSnapshotTest";

  exahype2::plotter::Snapshot snapshot;
  snapshot.openFile<tarch::plotter::VTUFileWriter, tarch::plotter::PVDTimeSeriesWriter>(testDirectory(), 0.25);
  plotOnePoint(snapshot.getFileWriter());
  snapshot.writeFile(testDirectory(), filename);

  const std::filesystem::path path = std::filesystem::path(testDirectory()) / snapshotFileName(filename, 0.25);
  validateTrue(std::filesystem::exists(path));
  validateTrue(readFile(path).find(R"(<Piece NumberOfPoints="1" NumberOfCells="1">)") != std::string::npos);

  removeSnapshotFiles(filename, {0.25});
}

/**
 * Checks that the writers of two snapshots write two files and that the meta file lists both.
 */
void exahype2::plotter::tests::SnapshotTest::testCollectsEverySnapshotInTheMetaFile() {
  const std::string filename = "ExaHyPE2PlotterSnapshotSeriesTest";

  exahype2::plotter::Snapshot snapshot;
  for (const double time : {0.5, 1.5}) {
    snapshot.openFile<tarch::plotter::VTUFileWriter, tarch::plotter::PVDTimeSeriesWriter>(testDirectory(), time);
    plotOnePoint(snapshot.getFileWriter());
    snapshot.writeFile(testDirectory(), filename);
  }

  validateTrue(std::filesystem::exists(std::filesystem::path(testDirectory()) / snapshotFileName(filename, 0.5)));
  validateTrue(std::filesystem::exists(std::filesystem::path(testDirectory()) / snapshotFileName(filename, 1.5)));

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string metaFile = readFile(std::filesystem::path(testDirectory()) / (filename + ".pvd"));
    validateTrue(metaFile.find("timestep=\"0.5\"") != std::string::npos);
    validateTrue(metaFile.find("timestep=\"1.5\"") != std::string::npos);
  }

  removeSnapshotFiles(filename, {0.5, 1.5});
}
