// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/VTUFileWriter.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Mesh.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/plotter/Meshes.h"

namespace {
  /**
   * The path, the existence and the content of a written file as a rank read it back.
   */
  struct ReadBackFile final {
    std::string path{};
    bool        exists{false};
    std::string content{};
  };

  /**
   * Stores the piece of this rank, the parallel file and the series file of the snapshot at the time 0.
   * Every rank other than the global master leaves the records of the parallel file and the series file
   * empty.
   */
  struct Snapshot final {
    ReadBackFile piece{};
    ReadBackFile parallelFile{};
    ReadBackFile seriesFile{};
  };

  ReadBackFile readAndRemove(const std::string& path) {
    ReadBackFile file{.path = path, .exists = std::filesystem::exists(path)};
    if (file.exists) {
      const std::ifstream stream(path, std::ios::binary);
      std::stringstream   buffer;
      buffer << stream.rdbuf();
      file.content = buffer.str();
      std::filesystem::remove(path);
    }
    return file;
  }

  ReadBackFile readAndRemoveOnGlobalMaster(const std::string& path) {
    return tarch::mpi::Rank::getInstance().isGlobalMaster() ? readAndRemove(path) : ReadBackFile{};
  }

  std::string getPath(const std::string& name) { return (tarch::testing::getWritableTestDirectory() / name).string(); }

  std::string getPiecePath(const std::string& filename, const std::string& snapshotTime) {
    const int rank = tarch::mpi::Rank::getInstance().getRank();
    return getPath(filename + "-" + snapshotTime + "-rank-" + std::to_string(rank) + ".vtu");
  }

  /**
   * Writes mesh as the snapshot at the time 0, reads its files back and removes them.
   */
  Snapshot writeAndReadBack(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::VTUFileWriter writer;
    writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    Snapshot snapshot{
      .piece        = readAndRemove(getPiecePath(filename, "0_000000")),
      .parallelFile = readAndRemoveOnGlobalMaster(getPath(filename + "-0_000000.pvtu")),
      .seriesFile   = readAndRemoveOnGlobalMaster(getPath(filename + ".pvd"))
    };
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    return snapshot;
  }

  void validateFileContent(const ReadBackFile& file, const std::string& expectedSubstring) {
    VALIDATE(file.exists, file.path);
    VALIDATE(file.content.find(expectedSubstring) != std::string::npos, file.path, expectedSubstring);
  }

  /**
   * Checks the parallel file and the series file of the snapshot at the time 0 on the global master.
   */
  void validateSeries(const Snapshot& snapshot, const std::string& filename) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      validateFileContent(snapshot.parallelFile, "<VTKFile type=\"PUnstructuredGrid\"");
      validateFileContent(snapshot.seriesFile, "file=\"" + filename + "-0_000000.pvtu\"");
    }
  }
} // namespace

UNIT_TEST("A unit square writes a piece of four points and one cell with the cell types and a series of it") {
  GIVEN("A mesh of one unit square") {
    const std::string    filename = "VTUTest2DQuadrangle";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the ranks read the files back") {
      const Snapshot snapshot = writeAndReadBack(mesh, filename);

      THEN("The piece contains four points, one cell and the cell types and the global master wrote the series") {
        validateFileContent(snapshot.piece, R"(<Piece NumberOfPoints="4" NumberOfCells="1">)");
        validateFileContent(snapshot.piece, "Name=\"types\"");
        validateSeries(snapshot, filename);
      }
    }
  }
}

UNIT_TEST("A unit cube writes a piece of eight points and one cell and a series of it") {
  GIVEN("A mesh of one unit cube") {
    const std::string    filename = "VTUTest3DHexahedron";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitCube(mesh, 0.0);

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the ranks read the files back") {
      const Snapshot snapshot = writeAndReadBack(mesh, filename);

      THEN("The piece contains eight points and one cell and the global master wrote the series") {
        validateFileContent(snapshot.piece, R"(<Piece NumberOfPoints="8" NumberOfCells="1">)");
        validateSeries(snapshot, filename);
      }
    }
  }
}

UNIT_TEST("Five particles write their point fields and the writer pads a two-component field to three components") {
  GIVEN("Five points with the scalar Mass, the three-component Velocity and the two-component Velocity2D") {
    const std::string    filename = "VTUTest2DParticles";
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ = {0, 0, 0, 1, 1, 0, 2, 2, 0, 3, 3, 0, 4, 4, 0};
    mesh.pointFields.push_back({"Mass", 1, {0.0, 10.0, 20.0, 30.0, 40.0}});
    mesh.pointFields.push_back({"Velocity", 3, {1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1}});
    mesh.pointFields.push_back({"Velocity2D", 2, {2, 3, 2, 3, 2, 3, 2, 3, 2, 3}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the ranks read the files back") {
      const Snapshot snapshot = writeAndReadBack(mesh, filename);

      THEN("The piece contains five points and zero cells with the three fields, Velocity2D with three components") {
        validateFileContent(snapshot.piece, "<PointData>");
        validateFileContent(snapshot.piece, "Name=\"Mass\"");
        validateFileContent(snapshot.piece, R"(Name="Velocity" NumberOfComponents="3")");
        validateFileContent(snapshot.piece, R"(Name="Velocity2D" NumberOfComponents="3")");
        validateFileContent(snapshot.piece, R"(<Piece NumberOfPoints="5" NumberOfCells="0">)");
        validateSeries(snapshot, filename);
      }
    }
  }
}

UNIT_TEST("Two unit cubes write their cell field") {
  GIVEN("A mesh of two unit cubes with the scalar cell field Pressure") {
    const std::string    filename = "VTUTest3DCellData";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitCube(mesh, 0.0);
    tarch::plotter::unittests::appendUnitCube(mesh, 2.0);
    mesh.cellFields.push_back({"Pressure", 1, {101325.0, 0.0}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the ranks read the files back") {
      const Snapshot snapshot = writeAndReadBack(mesh, filename);

      THEN("The piece contains two cells and the cell field Pressure") {
        validateFileContent(snapshot.piece, "<CellData>");
        validateFileContent(snapshot.piece, "Name=\"Pressure\"");
        validateFileContent(snapshot.piece, "NumberOfCells=\"2\"");
        validateSeries(snapshot, filename);
      }
    }
  }
}

UNIT_TEST("A mesh without points, cells and fields writes a valid empty piece") {
  GIVEN("An empty mesh") {
    const std::string          filename = "VTUTestEmpty";
    const tarch::plotter::Mesh mesh;

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the ranks read the files back") {
      const Snapshot snapshot = writeAndReadBack(mesh, filename);

      THEN("The piece contains zero points and zero cells") {
        validateFileContent(snapshot.piece, "NumberOfPoints=\"0\"");
        validateFileContent(snapshot.piece, "NumberOfCells=\"0\"");
        validateSeries(snapshot, filename);
      }
    }
  }
}

UNIT_TEST("The series file lists every snapshot file that one writer writes once with its exact time") {
  GIVEN("A mesh of one unit square") {
    const std::string    filename = "VTUTestSeries";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);

    WHEN("One writer writes the mesh at the times 0, 1/3 and 1/3 and the global master reads the series back") {
      tarch::plotter::VTUFileWriter writer;
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 1.0 / 3.0);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 1.0 / 3.0);
      tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
      const ReadBackFile seriesFile = readAndRemoveOnGlobalMaster(getPath(filename + ".pvd"));
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        std::filesystem::remove(getPath(filename + "-0_000000.pvtu"));
        std::filesystem::remove(getPath(filename + "-0_333333.pvtu"));
      }
      std::filesystem::remove(getPiecePath(filename, "0_000000"));
      std::filesystem::remove(getPiecePath(filename, "0_333333"));
      tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

      THEN("The series file of the global master lists both snapshots once with their exact times") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          const std::string secondSnapshot = "VTUTestSeries-0_333333.pvtu";
          validateFileContent(seriesFile, R"(timestep="0" group="" part="0" file="VTUTestSeries-0_000000.pvtu")");
          validateFileContent(
            seriesFile,
            R"(timestep="0.3333333333333333" group="" part="0" file="VTUTestSeries-0_333333.pvtu")"
          );
          VALIDATE(
            seriesFile.content.find(secondSnapshot) == seriesFile.content.rfind(secondSnapshot),
            seriesFile.path
          );
        }
      }
    }
  }
}
