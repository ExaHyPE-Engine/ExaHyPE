// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/VTUFileWriter.h"

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

tarch::plotter::tests::VTUFileWriterTest::VTUFileWriterTest() :
  TestCase("tarch::plotter::tests::VTUFileWriterTest") {}

void tarch::plotter::tests::VTUFileWriterTest::run() {
  testMethod(test2DSimpleTriangle);
  testMethod(test3DSimpleHexahedron);
  testMethod(test2DParticlesWithData);
  testMethod(test3DCellData);

  testMethod(testEmptyPlotter);
  testMethod(testMixedEmptySubdomains);
  testMethod(testMultipleNonEmptySubdomains);
  testMethod(testOversizedDataThrows);
  testMethod(testInconsistentSchemaThrows);
  testMethod(testUnevenSubdomainCounts);
  testMethod(testInvalidIdentifierThrows);
}

void tarch::plotter::tests::VTUFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "VTUTest2DTriangle";

  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);

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

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  // Piece header and the types array must be present.
  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"3\" NumberOfCells=\"1\">");
  validateFileContent(vtuPath, "Name=\"types\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test3DSimpleHexahedron() {
  const std::string filename = "VTUTest3DHex";

  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
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

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"8\" NumberOfCells=\"1\">");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test2DParticlesWithData() {
  const std::string filename = "VTUTest2DParticles";

  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto          v   = plotter.createVertexPlotter(0);
  auto          vd  = plotter.createVertexDataPlotter(0, "Mass", 1);
  auto          vv  = plotter.createVertexDataPlotter(0, "Velocity", 3);   // 3D vector on 2D point
  auto          vv2 = plotter.createVertexDataPlotter(0, "Velocity2D", 2); // 2D vector, padded to 3

  for (int i = 0; i < 5; i++) {
    tarch::la::Vector<2, double> p   = {static_cast<double>(i), static_cast<double>(i)};
    int                          idx = v->plotVertex(p);
    vd->plotVertexData(idx, 10.0 * i); // Scalar mass
    tarch::la::Vector<3, double> vel = {1.0, 0.0, -1.0};
    vv->plotVertexData(idx, vel);                                     // Vector velocity
    vv2->plotVertexData(idx, tarch::la::Vector<2, double>{2.0, 3.0}); // 2D vector velocity
  }

  plotter.writeToFile(filename, 0);

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<PointData>");
  validateFileContent(vtuPath, "Name=\"Mass\"");
  validateFileContent(vtuPath, "Name=\"Velocity\" NumberOfComponents=\"3\"");
  // The 2-component field is padded to a 3-component vector.
  validateFileContent(vtuPath, "Name=\"Velocity2D\" NumberOfComponents=\"3\"");
  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"5\" NumberOfCells=\"0\">");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::test3DCellData() {
  const std::string filename = "VTUTest3DCellData";

  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto          v  = plotter.createVertexPlotter(0);
  auto          c  = plotter.createCellPlotter(0);
  auto          cd = plotter.createCellDataPlotter(0, "Pressure", 1);

  // Two triangles sharing an edge (5 points total).
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

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<CellData>");
  validateFileContent(vtuPath, "Name=\"Pressure\"");
  validateFileContent(vtuPath, "NumberOfCells=\"2\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testEmptyPlotter() {
  const std::string filename = "VTUTestEmpty";

  {
    VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    // Nothing plotted; just write.
    plotter.writeToFile(filename, 0);
  }

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  // File exists with 0 points/cells.
  validateFileContent(vtuPath, "NumberOfPoints=\"0\"");
  validateFileContent(vtuPath, "NumberOfCells=\"0\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testMixedEmptySubdomains() {
  const std::string filename = "VTUTestMixedEmpty";

  // Two subdomains (e.g. threads or local partitions).
  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 2);

  // Subdomain 0: has data
  auto v0      = plotter.createVertexPlotter(0);
  int  i0      = v0->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int  i1      = v0->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
  auto c0      = plotter.createCellPlotter(0);
  int  line[2] = {i0, i1};
  c0->plotLine(line);

  // Subdomain 1: empty (we never create plotters for it)

  plotter.writeToFile(filename, 0);

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  // Must contain subdomain 0's data and ignore the empty subdomain 1.
  validateFileContent(vtuPath, "NumberOfPoints=\"2\"");
  validateFileContent(vtuPath, "NumberOfCells=\"1\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testMultipleNonEmptySubdomains() {
  const std::string filename = "VTUTestMultipleSubdomains";

  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 2);

  {
    auto v = plotter.createVertexPlotter(0);
    auto c = plotter.createCellPlotter(0);

    int p0          = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int p1          = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int p2          = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int triangle[3] = {p0, p1, p2};
    c->plotTriangle(triangle);
  }

  {
    auto v = plotter.createVertexPlotter(1);
    auto c = plotter.createCellPlotter(1);

    int p0      = v->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
    int p1      = v->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
    int p2      = v->plotVertex(tarch::la::Vector<2, double>{3.0, 1.0});
    int p3      = v->plotVertex(tarch::la::Vector<2, double>{2.0, 1.0});
    int quad[4] = {p0, p1, p2, p3};
    c->plotQuadrangle(quad);
  }

  plotter.writeToFile(filename, 0);

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"3\" NumberOfCells=\"1\">");
  validateFileContent(vtuPath, "<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }

  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testOversizedDataThrows() {
  const std::string filename = "VTUTestOversized";

  bool exceptionThrown = false;
  try {
    VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto          v = plotter.createVertexPlotter(0);

    int i0 = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});

    // One vertex, but data is plotted for two.
    auto vd = plotter.createVertexDataPlotter(0, "ErrorField", 1);
    vd->plotVertexData(i0, 1.0);
    vd->plotVertexData(i0 + 1, 2.0); // extra data

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  // Clean up if the file was created despite the expected throw.
  if (fs::exists(vtuPath)) {
    fs::remove(vtuPath);
  }

  validate(exceptionThrown);
}

void tarch::plotter::tests::VTUFileWriterTest::testInconsistentSchemaThrows() {
  const std::string filename = "VTUTestSchemaMismatch";

  bool exceptionThrown = false;
  try {
    VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 2);

    // Subdomain 0: "Pressure" as scalar.
    auto v0 = plotter.createVertexPlotter(0);
    v0->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    auto vd0 = plotter.createVertexDataPlotter(0, "Pressure", 1);
    vd0->plotVertexData(0, 10.0);

    // Subdomain 1: "Pressure" as 3D vector -> mismatch.
    auto v1 = plotter.createVertexPlotter(1);
    v1->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    auto vd1 = plotter.createVertexDataPlotter(1, "Pressure", 3);
    vd1->plotVertexData(0, tarch::la::Vector<3, double>{0.0, 0.0, 0.0});

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  if (fs::exists(vtuPath)) {
    fs::remove(vtuPath);
  }

  validate(exceptionThrown);
}

void tarch::plotter::tests::VTUFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "VTUTestUneven";

  // Master holds 3 subdomains, every other rank 2. VTU writes one <Piece> per
  // subdomain into this rank's own .vtu, so the file must hold exactly nSub pieces.
  const int     nSub = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), nSub);
  for (int sd = 0; sd < nSub; ++sd) {
    auto         v       = plotter.createVertexPlotter(sd);
    auto         c       = plotter.createCellPlotter(sd);
    const double s       = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * sd;
    int          p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
    int          p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
    int          p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
    int          p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
    int          quad[4] = {p0, p1, p2, p3};
    c->plotQuadrangle(quad);
  }

  plotter.writeToFile(filename, 0);

  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  std::ifstream     f(vtuPath);
  std::stringstream buffer;
  buffer << f.rdbuf();
  const std::string content = buffer.str();
  int               pieces  = 0;
  for (std::size_t pos = content.find("<Piece "); pos != std::string::npos; pos = content.find("<Piece ", pos + 1)) {
    ++pieces;
  }
  if (pieces != nSub) {
    throw std::runtime_error(
      "VTU uneven test: expected " + std::to_string(nSub) + " pieces in " + vtuPath + " but found " +
      std::to_string(pieces) + "."
    );
  }

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    fs::remove(pvtuPath);
  }
  fs::remove(vtuPath);
}

void tarch::plotter::tests::VTUFileWriterTest::testInvalidIdentifierThrows() {
  VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  bool          thrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  if (not thrown) {
    throw std::runtime_error("Expected the illegal data identifier 'bad/name' to be rejected.");
  }
}
