// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriterTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/VTUFileWriter.h"
#include "tarch/plotter/tests/Meshes.h"

namespace {
  /**
   * Throws when the file does not exist or does not contain the substring.
   */
  void validateFileContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("The test expects the file '" + filepath + "'.");
    }

    const std::ifstream stream(filepath, std::ios::binary);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error("The test expects '" + expectedSubstring + "' in the file '" + filepath + "'.");
    }
  }

  std::string getPath(const std::string& name) { return (tarch::tests::getWritableTestDirectory() / name).string(); }

  /**
   * Writes mesh as the snapshot at the time 0 and returns the path of the .vtu file of this rank.
   */
  std::string writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::VTUFileWriter writer;
    writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
    return getPath(filename + "-0_000000-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank()) + ".vtu");
  }

  /**
   * Checks the .pvtu file of the snapshot at the time 0 and removes the files of the series filename.
   */
  void validateAndRemoveSeries(const std::string& filename, const std::string& vtuPath) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      validateFileContent(getPath(filename + "-0_000000.pvtu"), "<VTKFile type=\"PUnstructuredGrid\"");
      validateFileContent(getPath(filename + ".pvd"), "file=\"" + filename + "-0_000000.pvtu\"");
      std::filesystem::remove(getPath(filename + "-0_000000.pvtu"));
      std::filesystem::remove(getPath(filename + ".pvd"));
    }
    std::filesystem::remove(vtuPath);
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  }
} // namespace

tarch::plotter::tests::VTUFileWriterTest::VTUFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::VTUFileWriterTest") {}

void
tarch::plotter::tests::VTUFileWriterTest::run() {
  // The read-back helpers throw for a malformed file. An exception that leaves run() ends the test
  // executable.
  try {
    testMethod(test2DSimpleQuadrangle);
    testMethod(test3DSimpleHexahedron);
    testMethod(test2DParticlesWithData);
    testMethod(test3DCellData);
    testMethod(testEmptyMesh);
    testMethod(testSeries);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

void
tarch::plotter::tests::VTUFileWriterTest::test2DSimpleQuadrangle() {
  const std::string filename = "VTUTest2DQuadrangle";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  const std::string vtuPath = writeMesh(mesh, filename);

  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="4" NumberOfCells="1">)");
  validateFileContent(vtuPath, "Name=\"types\"");
  validateAndRemoveSeries(filename, vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test3DSimpleHexahedron() {
  const std::string filename = "VTUTest3DHexahedron";

  tarch::plotter::Mesh mesh;
  appendUnitCube(mesh, 0.0);
  const std::string vtuPath = writeMesh(mesh, filename);

  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="8" NumberOfCells="1">)");
  validateAndRemoveSeries(filename, vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test2DParticlesWithData() {
  const std::string filename = "VTUTest2DParticles";

  tarch::plotter::Mesh mesh;
  mesh.pointsXYZ = {0, 0, 0, 1, 1, 0, 2, 2, 0, 3, 3, 0, 4, 4, 0};
  mesh.pointFields.push_back({"Mass", 1, {0.0, 10.0, 20.0, 30.0, 40.0}});
  mesh.pointFields.push_back({"Velocity", 3, {1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1}});
  // The writer pads a two-component field to three components.
  mesh.pointFields.push_back({"Velocity2D", 2, {2, 3, 2, 3, 2, 3, 2, 3, 2, 3}});
  const std::string vtuPath = writeMesh(mesh, filename);

  validateFileContent(vtuPath, "<PointData>");
  validateFileContent(vtuPath, "Name=\"Mass\"");
  validateFileContent(vtuPath, R"(Name="Velocity" NumberOfComponents="3")");
  validateFileContent(vtuPath, R"(Name="Velocity2D" NumberOfComponents="3")");
  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="5" NumberOfCells="0">)");
  validateAndRemoveSeries(filename, vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test3DCellData() {
  const std::string filename = "VTUTest3DCellData";

  tarch::plotter::Mesh mesh;
  appendUnitCube(mesh, 0.0);
  appendUnitCube(mesh, 2.0);
  mesh.cellFields.push_back({"Pressure", 1, {101325.0, 0.0}});
  const std::string vtuPath = writeMesh(mesh, filename);

  validateFileContent(vtuPath, "<CellData>");
  validateFileContent(vtuPath, "Name=\"Pressure\"");
  validateFileContent(vtuPath, "NumberOfCells=\"2\"");
  validateAndRemoveSeries(filename, vtuPath);
}

/**
 * A mesh without points, cells and fields gives a valid empty file.
 */
void
tarch::plotter::tests::VTUFileWriterTest::testEmptyMesh() {
  const std::string filename = "VTUTestEmpty";

  const std::string vtuPath = writeMesh(tarch::plotter::Mesh(), filename);

  validateFileContent(vtuPath, "NumberOfPoints=\"0\"");
  validateFileContent(vtuPath, "NumberOfCells=\"0\"");
  validateAndRemoveSeries(filename, vtuPath);
}

/**
 * The .pvd file lists every snapshot file that one writer writes once with its exact time.
 */
void
tarch::plotter::tests::VTUFileWriterTest::testSeries() {
  const std::string filename = "VTUTestSeries";
  const std::string rank     = std::to_string(tarch::mpi::Rank::getInstance().getRank());

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  tarch::plotter::VTUFileWriter writer;
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 1.0 / 3.0);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 1.0 / 3.0);

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileContent(
      getPath(filename + ".pvd"),
      R"(timestep="0" group="" part="0" file="VTUTestSeries-0_000000.pvtu")"
    );
    validateFileContent(
      getPath(filename + ".pvd"),
      R"(timestep="0.3333333333333333" group="" part="0" file="VTUTestSeries-0_333333.pvtu")"
    );
    const std::ifstream stream(getPath(filename + ".pvd"));
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();
    if (content.find("VTUTestSeries-0_333333.pvtu") != content.rfind("VTUTestSeries-0_333333.pvtu")) {
      throw std::runtime_error("The test expects the snapshot VTUTestSeries-0_333333.pvtu once in the file VTUTestSeries.pvd.");
    }
    std::filesystem::remove(getPath(filename + "-0_000000.pvtu"));
    std::filesystem::remove(getPath(filename + "-0_333333.pvtu"));
    std::filesystem::remove(getPath(filename + ".pvd"));
  }
  std::filesystem::remove(getPath(filename + "-0_000000-rank-" + rank + ".vtu"));
  std::filesystem::remove(getPath(filename + "-0_333333-rank-" + rank + ".vtu"));
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}
