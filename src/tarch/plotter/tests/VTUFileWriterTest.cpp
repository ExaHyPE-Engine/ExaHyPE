// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/VTUFileWriter.h"

namespace {
  /**
   * Throws if the file does not exist or does not contain the substring.
   */
  void validateFileContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("Test failed: Output file was not created: " + filepath);
    }

    const std::ifstream stream(filepath, std::ios::binary);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error(
        "Test failed: Output file '" + filepath + "' does not contain expected string: '" + expectedSubstring + "'"
      );
    }
  }
} // namespace

tarch::plotter::tests::VTUFileWriterTest::VTUFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::VTUFileWriterTest") {}

void
tarch::plotter::tests::VTUFileWriterTest::run() {
  // The read-back helpers report a malformed file by throwing, and the test framework does not
  // guard run(), so this catch records the failure and keeps the binary alive.
  try {
    testMethod(test2DSimpleTriangle);
    testMethod(test3DSimpleHexahedron);
    testMethod(test2DParticlesWithData);
    testMethod(test3DCellData);
    testMethod(testCellTypeMetadata);

    testMethod(testEmptyPlotter);
    testMethod(testMixedEmptySubdomains);
    testMethod(testMultipleNonEmptySubdomains);
    testMethod(testOversizedDataThrows);
    testMethod(testInconsistentSchemaThrows);
    testMethod(testUnevenSubdomainCounts);
    testMethod(testInvalidIdentifierThrows);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

void
tarch::plotter::tests::VTUFileWriterTest::testCellTypeMetadata() {
  const auto* const triangle = tarch::plotter::lookupCellType(tarch::plotter::CellType::Triangle);
  validate(triangle == tarch::plotter::lookupCellType(static_cast<int>(tarch::plotter::CellType::Triangle)));
  validate(tarch::plotter::lookupCellType(-1) == nullptr);
  validate(triangle->type == tarch::plotter::CellType::Triangle);
  validateEquals(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Triangle), 3);
  validate(tarch::plotter::xdmfTypeNeedsNodeCount(2));
  validate(not tarch::plotter::xdmfTypeNeedsNodeCount(triangle->xdmf));
}

void
tarch::plotter::tests::VTUFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "VTUTest2DTriangle";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);

  auto                               vertexPlotter = plotter.createVertexPlotter(0);
  const tarch::la::Vector<2, double> firstCorner{0.0, 0.0};
  const tarch::la::Vector<2, double> secondCorner{1.0, 0.0};
  const tarch::la::Vector<2, double> thirdCorner{0.0, 1.0};

  const int firstIndex  = vertexPlotter->plotVertex(firstCorner);
  const int secondIndex = vertexPlotter->plotVertex(secondCorner);
  const int thirdIndex  = vertexPlotter->plotVertex(thirdCorner);

  auto      cellPlotter = plotter.createCellPlotter(0);
  const int indices[3]  = {firstIndex, secondIndex, thirdIndex};
  cellPlotter->plotTriangle(indices);

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="3" NumberOfCells="1">)");
  validateFileContent(vtuPath, "Name=\"types\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test3DSimpleHexahedron() {
  const std::string filename = "VTUTest3DHex";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter = plotter.createVertexPlotter(0);

  // The bottom face comes first and the top face follows in the same orientation.
  int cornerIndices[8] = {};
  cornerIndices[0]     = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 0, 0});
  cornerIndices[1]     = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1, 0, 0});
  cornerIndices[2]     = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1, 1, 0});
  cornerIndices[3]     = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 1, 0});

  cornerIndices[4] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 0, 1});
  cornerIndices[5] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1, 0, 1});
  cornerIndices[6] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1, 1, 1});
  cornerIndices[7] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 1, 1});

  auto cellPlotter = plotter.createCellPlotter(0);
  cellPlotter->plotHexahedron(cornerIndices);

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="8" NumberOfCells="1">)");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test2DParticlesWithData() {
  const std::string filename = "VTUTest2DParticles";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter   = plotter.createVertexPlotter(0);
  auto massPlotter     = plotter.createVertexDataPlotter(0, "Mass", 1);
  auto velocityPlotter = plotter.createVertexDataPlotter(0, "Velocity", 3);
  // The writer pads a two-component field to three components.
  auto planarVelocityPlotter = plotter.createVertexDataPlotter(0, "Velocity2D", 2);

  for (int particleIndex = 0; particleIndex < 5; particleIndex++) {
    const tarch::la::Vector<2, double> position{static_cast<double>(particleIndex), static_cast<double>(particleIndex)};
    const int                          vertexIndex = vertexPlotter->plotVertex(position);
    massPlotter->plotVertexData(vertexIndex, 10.0 * particleIndex);
    const tarch::la::Vector<3, double> velocity{1.0, 0.0, -1.0};
    velocityPlotter->plotVertexData(vertexIndex, velocity);
    planarVelocityPlotter->plotVertexData(vertexIndex, tarch::la::Vector<2, double>{2.0, 3.0});
  }

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<PointData>");
  validateFileContent(vtuPath, "Name=\"Mass\"");
  validateFileContent(vtuPath, R"(Name="Velocity" NumberOfComponents="3")");
  validateFileContent(vtuPath, R"(Name="Velocity2D" NumberOfComponents="3")");
  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="5" NumberOfCells="0">)");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::test3DCellData() {
  const std::string filename = "VTUTest3DCellData";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter   = plotter.createVertexPlotter(0);
  auto cellPlotter     = plotter.createCellPlotter(0);
  auto pressurePlotter = plotter.createCellDataPlotter(0, "Pressure", 1);

  // The two triangles meet along the edge between vertex 1 and vertex 2.
  int vertexIndices[5];
  vertexIndices[0] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 0, 0});
  vertexIndices[1] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1, 0, 0});
  vertexIndices[2] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 1, 0});
  vertexIndices[3] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 0, 1});
  vertexIndices[4] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0, 0, -1});

  const int firstTriangle[3] = {vertexIndices[0], vertexIndices[1], vertexIndices[2]};
  const int firstCellIndex   = cellPlotter->plotTriangle(firstTriangle);
  pressurePlotter->plotCellData(firstCellIndex, 101325.0);

  const int secondTriangle[3] = {vertexIndices[1], vertexIndices[2], vertexIndices[3]};
  const int secondCellIndex   = cellPlotter->plotTriangle(secondTriangle);
  pressurePlotter->plotCellData(secondCellIndex, 0.0);

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "<CellData>");
  validateFileContent(vtuPath, "Name=\"Pressure\"");
  validateFileContent(vtuPath, "NumberOfCells=\"2\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::testEmptyPlotter() {
  const std::string filename = "VTUTestEmpty";

  {
    tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    plotter.writeToFile(filename, 0);
  }

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "NumberOfPoints=\"0\"");
  validateFileContent(vtuPath, "NumberOfCells=\"0\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::testMixedEmptySubdomains() {
  const std::string filename = "VTUTestMixedEmpty";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  plotter.acquirePiece(1);

  auto      vertexPlotter = plotter.createVertexPlotter(0);
  const int firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  const int secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
  auto      cellPlotter   = plotter.createCellPlotter(0);
  const int line[2]       = {firstIndex, secondIndex};
  cellPlotter->plotLine(line);

  // Subdomain 1 keeps no plotter, so it stays empty.

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, "NumberOfPoints=\"2\"");
  validateFileContent(vtuPath, "NumberOfCells=\"1\"");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::testMultipleNonEmptySubdomains() {
  const std::string filename = "VTUTestMultipleSubdomains";

  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  plotter.acquirePiece(1);

  {
    auto vertexPlotter = plotter.createVertexPlotter(0);
    auto cellPlotter   = plotter.createCellPlotter(0);

    const int firstIndex  = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    const int secondIndex = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    const int thirdIndex  = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    const int triangle[3] = {firstIndex, secondIndex, thirdIndex};
    cellPlotter->plotTriangle(triangle);
  }

  {
    auto vertexPlotter = plotter.createVertexPlotter(1);
    auto cellPlotter   = plotter.createCellPlotter(1);

    const int firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
    const int secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
    const int thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{3.0, 1.0});
    const int fourthIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.0, 1.0});
    const int quadrangle[4] = {firstIndex, secondIndex, thirdIndex, fourthIndex};
    cellPlotter->plotQuadrangle(quadrangle);
  }

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="3" NumberOfCells="1">)");
  validateFileContent(vtuPath, R"(<Piece NumberOfPoints="4" NumberOfCells="1">)");

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }

  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::testOversizedDataThrows() {
  const std::string filename = "VTUTestOversized";

  bool exceptionThrown = false;
  try {
    tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter = plotter.createVertexPlotter(0);

    const int vertexIndex = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});

    // The data plotter writes two values while the piece holds one vertex.
    auto dataPlotter = plotter.createVertexDataPlotter(0, "ErrorField", 1);
    dataPlotter->plotVertexData(vertexIndex, 1.0);
    dataPlotter->plotVertexData(vertexIndex + 1, 2.0);

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  if (std::filesystem::exists(vtuPath)) {
    std::filesystem::remove(vtuPath);
  }

  validate(exceptionThrown);
}

void
tarch::plotter::tests::VTUFileWriterTest::testInconsistentSchemaThrows() {
  const std::string filename = "VTUTestSchemaMismatch";

  bool exceptionThrown = false;
  try {
    tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    plotter.acquirePiece(1);

    auto firstVertexPlotter = plotter.createVertexPlotter(0);
    firstVertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    auto scalarPressurePlotter = plotter.createVertexDataPlotter(0, "Pressure", 1);
    scalarPressurePlotter->plotVertexData(0, 10.0);

    // Subdomain 1 declares Pressure with three components, which mismatches subdomain 0.
    auto secondVertexPlotter = plotter.createVertexPlotter(1);
    secondVertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    auto vectorPressurePlotter = plotter.createVertexDataPlotter(1, "Pressure", 3);
    vectorPressurePlotter->plotVertexData(0, tarch::la::Vector<3, double>{0.0, 0.0, 0.0});

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  if (std::filesystem::exists(vtuPath)) {
    std::filesystem::remove(vtuPath);
  }

  validate(exceptionThrown);
}

void
tarch::plotter::tests::VTUFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "VTUTestUneven";

  // The master plots three subdomains and every other rank two, and the writer puts one piece per
  // subdomain into the .vtu file of the rank.
  const int                     numberOfSubdomains = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  for (int subdomainIndex = 0; subdomainIndex < numberOfSubdomains; ++subdomainIndex) {
    plotter.acquirePiece(subdomainIndex);
  }
  for (int subdomainIndex = 0; subdomainIndex < numberOfSubdomains; ++subdomainIndex) {
    auto         vertexPlotter = plotter.createVertexPlotter(subdomainIndex);
    auto         cellPlotter   = plotter.createCellPlotter(subdomainIndex);
    const double offset        = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * subdomainIndex;
    const int    firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
    const int    secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
    const int    thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 1.0});
    const int    fourthIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
    const int    quadrangle[4] = {firstIndex, secondIndex, thirdIndex, fourthIndex};
    cellPlotter->plotQuadrangle(quadrangle);
  }

  plotter.writeToFile(filename, 0);

  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const std::string vtuPath =
    (tarch::tests::getWritableTestDirectory() / (filename + "-rank-" + std::to_string(rank) + ".vtu")).string();

  const std::ifstream stream(vtuPath);
  std::stringstream   buffer;
  buffer << stream.rdbuf();
  const std::string content        = buffer.str();
  int               numberOfPieces = 0;
  for (std::size_t position = content.find("<Piece "); position != std::string::npos;
       position             = content.find("<Piece ", position + 1)) {
    ++numberOfPieces;
  }
  if (numberOfPieces != numberOfSubdomains) {
    throw std::runtime_error(
      "VTU uneven test: Expected " + std::to_string(numberOfSubdomains) + " pieces in " + vtuPath + " but found "
      + std::to_string(numberOfPieces) + "."
    );
  }

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::string pvtuPath = (tarch::tests::getWritableTestDirectory() / (filename + ".pvtu")).string();
    validateFileContent(pvtuPath, "<VTKFile type=\"PUnstructuredGrid\"");
    std::filesystem::remove(pvtuPath);
  }
  std::filesystem::remove(vtuPath);
}

void
tarch::plotter::tests::VTUFileWriterTest::testInvalidIdentifierThrows() {
  tarch::plotter::VTUFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  bool exceptionThrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }
  if (not exceptionThrown) {
    throw std::runtime_error("Expected the illegal data identifier 'bad/name' to be rejected.");
  }
}
