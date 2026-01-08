// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/AMRPlotter.h"
#include "tarch/plotter/VTUFileWriter.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {
  /**
   * Checks if a file exists and contains a specific substring (e.g., an XML tag).
   */
  void validateFileContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not fs::exists(filepath)) {
      throw std::runtime_error("Test failed: Output file was not created: " + filepath);
    }

    std::ifstream     f(filepath, std::ios::binary);
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error(
        "Test failed: Output file '" + filepath + "' does not contain expected string: '" + expectedSubstring + "'"
      );
    }
  }
} // namespace

tarch::plotter::tests::VTUFileWriterTest::VTUFileWriterTest():
  TestCase("tarch::plotter::tests::VTUFileWriterTest") {}

void tarch::plotter::tests::VTUFileWriterTest::run() {
  testMethod(test2DSimpleTriangle);
  testMethod(test3DSimpleHexahedron);
  testMethod(test2DParticlesWithData);
  testMethod(test3DCellData);
  testMethod(test2DAMRGrid);
  testMethod(test3DAMRGrid);

  testMethod(testEmptyPlotter);
  testMethod(testMixedEmptySubdomains);
  testMethod(testOversizedDataThrows);
  testMethod(testInconsistentSchemaThrows);
}

void tarch::plotter::tests::VTUFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "VTUTest2DTriangle";

  VTUFileWriter plotter("./", 1);

  auto v = plotter.createVertexPlotter(0);
  // Triangle (0,0), (1,0), (0,1)
  tarch::la::Vector<2, double> p0 = {0.0, 0.0};
  tarch::la::Vector<2, double> p1 = {1.0, 0.0};
  tarch::la::Vector<2, double> p2 = {0.0, 1.0};

  int i0 = v->plotVertex(p0);
  int i1 = v->plotVertex(p1);
  int i2 = v->plotVertex(p2);

  auto c          = plotter.createCellPlotter(0);
  int  indices[3] = {i0, i1, i2};
  c->plotTriangle(indices);

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  // Check for the VTK XML tag and correct piece info
  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"3\" NumberOfCells=\"1\">");
  validateFileContent(vtuPath, "Name=\"types\""); // Check types array exists

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test3DSimpleHexahedron() {
  const std::string filename = "VTUTest3DHex";

  VTUFileWriter plotter("./", 1);
  auto          v = plotter.createVertexPlotter(0);

  // Unit Cube
  int idx[8];
  idx[0] = v->plotVertex(tarch::la::Vector<3, double>{0, 0, 0});
  idx[1] = v->plotVertex(tarch::la::Vector<3, double>{1, 0, 0});
  idx[2] = v->plotVertex(tarch::la::Vector<3, double>{1, 1, 0});
  idx[3] = v->plotVertex(tarch::la::Vector<3, double>{0, 1, 0});

  idx[4] = v->plotVertex(tarch::la::Vector<3, double>{0, 0, 1});
  idx[5] = v->plotVertex(tarch::la::Vector<3, double>{1, 0, 1});
  idx[6] = v->plotVertex(tarch::la::Vector<3, double>{1, 1, 1});
  idx[7] = v->plotVertex(tarch::la::Vector<3, double>{0, 1, 1});

  auto c = plotter.createCellPlotter(0);
  c->plotHexahedron(idx);

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"8\" NumberOfCells=\"1\">");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test2DParticlesWithData() {
  const std::string filename = "VTUTest2DParticles";

  VTUFileWriter plotter("./", 1);
  auto          v  = plotter.createVertexPlotter(0);
  auto          vd = plotter.createVertexDataPlotter(0, "Mass", 1);
  auto          vv = plotter.createVertexDataPlotter(0, "Velocity", 3); // 3D vector on 2D point

  for (int i = 0; i < 5; i++) {
    tarch::la::Vector<2, double> p   = {static_cast<double>(i), static_cast<double>(i)};
    int                          idx = v->plotVertex(p);
    vd->plotVertexData(idx, 10.0 * i); // Scalar mass
    tarch::la::Vector<3, double> vel = {1.0, 0.0, -1.0};
    vv->plotVertexData(idx, vel); // Vector velocity
  }

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  validateFileContent(vtuPath, "<PointData>");
  validateFileContent(vtuPath, "Name=\"Mass\"");
  validateFileContent(vtuPath, "Name=\"Velocity\" NumberOfComponents=\"3\"");
  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"5\" NumberOfCells=\"0\">");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test3DCellData() {
  const std::string filename = "VTUTest3DCellData";

  VTUFileWriter plotter("./", 1);
  auto          v  = plotter.createVertexPlotter(0);
  auto          c  = plotter.createCellPlotter(0);
  auto          cd = plotter.createCellDataPlotter(0, "Pressure", 1);

  // Create two tetrahedrons sharing a face (5 points total)
  // Points
  int p[5];
  p[0] = v->plotVertex(tarch::la::Vector<3, double>{0, 0, 0});
  p[1] = v->plotVertex(tarch::la::Vector<3, double>{1, 0, 0});
  p[2] = v->plotVertex(tarch::la::Vector<3, double>{0, 1, 0});
  p[3] = v->plotVertex(tarch::la::Vector<3, double>{0, 0, 1});  // Top 1
  p[4] = v->plotVertex(tarch::la::Vector<3, double>{0, 0, -1}); // Bottom 1

  int t1[3]    = {p[0], p[1], p[2]};
  int cellIdx1 = c->plotTriangle(t1);
  cd->plotCellData(cellIdx1, 101325.0); // High pressure

  int t2[3]    = {p[1], p[2], p[3]};
  int cellIdx2 = c->plotTriangle(t2);
  cd->plotCellData(cellIdx2, 0.0); // Vacuum

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  validateFileContent(vtuPath, "<CellData>");
  validateFileContent(vtuPath, "Name=\"Pressure\"");
  validateFileContent(vtuPath, "NumberOfCells=\"2\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test2DAMRGrid() {
  const std::string filename = "VTUTest2DAMR";

  VTUFileWriter plotter("./", 1);
  auto          dataPlotter = plotter.createCellDataPlotter(0, "RefinementLevel", 1);

  AMRPlotter amrPlotter(plotter, 0, 0.5, 0.1);

  int idx = amrPlotter.plotCell2D({0.0, 0.0}, {1.0, 1.0}, 0);
  dataPlotter->plotCellData(idx, 0.0);

  // Fine Cells (Level 1)
  idx = amrPlotter.plotCell2D({1.0, 0.0}, {0.5, 0.5}, 1);
  dataPlotter->plotCellData(idx, 1.0);
  idx = amrPlotter.plotCell2D({1.5, 0.0}, {0.5, 0.5}, 1);
  dataPlotter->plotCellData(idx, 1.0);
  idx = amrPlotter.plotCell2D({1.0, 0.5}, {0.5, 0.5}, 1);
  dataPlotter->plotCellData(idx, 1.0);
  idx = amrPlotter.plotCell2D({1.5, 0.5}, {0.5, 0.5}, 1);
  dataPlotter->plotCellData(idx, 1.0);

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  validateFileContent(vtuPath, "NumberOfCells=\"5\""); // 1 + 4
  validateFileContent(vtuPath, "Name=\"RefinementLevel\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test3DAMRGrid() {
  const std::string filename = "VTUTest3DAMR";

  VTUFileWriter plotter("./", 1);
  auto          dataPlotter = plotter.createCellDataPlotter(0, "RefinementLevel", 1);

  AMRPlotter amrPlotter(plotter, 0, 0.5, 0.1);

  // One large block at origin (Level 0)
  int idx = amrPlotter.plotCell3D({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, 0);
  dataPlotter->plotCellData(idx, 0.0);

  // A refined block next to it (Level 1)
  // Logical position: x=1.0 to 1.5
  idx = amrPlotter.plotCell3D({1.0, 0.0, 0.0}, {0.5, 0.5, 0.5}, 1);
  dataPlotter->plotCellData(idx, 1.0);

  // Even more refined block (Level 2)
  // Logical position: x=1.0, y=0.5
  idx = amrPlotter.plotCell3D({1.0, 0.5, 0.0}, {0.25, 0.25, 0.25}, 2);
  dataPlotter->plotCellData(idx, 2.0);

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  validateFileContent(vtuPath, "NumberOfCells=\"3\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testEmptyPlotter() {
  const std::string filename = "VTUTestEmpty";

  {
    VTUFileWriter plotter("./", 1);
    // Do nothing, just close/write
    plotter.writeToFile(filename, 0);
  }

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  // Validate it created a file with 0 points/cells
  validateFileContent(vtuPath, "NumberOfPoints=\"0\"");
  validateFileContent(vtuPath, "NumberOfCells=\"0\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testMixedEmptySubdomains() {
  const std::string filename = "VTUTestMixedEmpty";

  // Simulate 2 subdomains (e.g. threads or local partitions)
  VTUFileWriter plotter("./", 2);

  // Subdomain 0: HAS DATA
  auto v0      = plotter.createVertexPlotter(0);
  int  i0      = v0->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int  i1      = v0->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
  auto c0      = plotter.createCellPlotter(0);
  int  line[2] = {i0, i1};
  c0->plotLine(line);

  // Subdomain 1: EMPTY (we never create plotters for it)

  plotter.writeToFile(filename, 0);

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  // Validate: Should contain the data from SD 0 and ignore SD 1
  validateFileContent(vtuPath, "NumberOfPoints=\"2\"");
  validateFileContent(vtuPath, "NumberOfCells=\"1\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = std::string("./") + filename + ".pvtu";
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testOversizedDataThrows() {
  const std::string filename = "VTUTestOversized";

  bool exceptionThrown = false;
  try {
    VTUFileWriter plotter("./", 1);
    auto          v = plotter.createVertexPlotter(0);

    int i0 = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});

    // We have 1 vertex, but we try to plot data for 2 vertices
    auto vd = plotter.createVertexDataPlotter(0, "ErrorField", 1);
    vd->plotVertexData(i0, 1.0);
    vd->plotVertexData(i0 + 1, 2.0); // Extra data

    plotter.writeToFile(filename, 0);

  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  // If the file was somehow created (test failed), clean it up
  if (fs::exists(vtuPath)) {
    fs::remove(vtuPath);
  }

  validate(exceptionThrown);
}

void tarch::plotter::tests::VTUFileWriterTest::testInconsistentSchemaThrows() {
  const std::string filename = "VTUTestSchemaMismatch";

  bool exceptionThrown = false;
  try {
    VTUFileWriter plotter("./", 2);

    // Subdomain 0: Defines "Pressure" (Scalar)
    auto v0 = plotter.createVertexPlotter(0);
    v0->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    auto vd0 = plotter.createVertexDataPlotter(0, "Pressure", 1);
    vd0->plotVertexData(0, 10.0);

    // Subdomain 1: Defines "Pressure" (Vector 3D) -> Mismatch!
    auto v1 = plotter.createVertexPlotter(1);
    v1->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    auto vd1 = plotter.createVertexDataPlotter(1, "Pressure", 3);
    vd1->plotVertexData(0, tarch::la::Vector<3, double>{0.0, 0.0, 0.0});

    plotter.writeToFile(filename, 0);

  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int   rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath = std::string("./") + filename + "-rank-" + std::to_string(rank) + ".vtu";

  if (fs::exists(vtuPath)) {
    fs::remove(vtuPath);
  }

  validate(exceptionThrown);
}
