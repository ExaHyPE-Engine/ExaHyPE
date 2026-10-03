// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/NetCDFFileWriter.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>

namespace {
  void checkNc(const int status, const std::string& where) {
    if (status != NC_NOERR) {
      throw std::runtime_error("NetCDF test error in " + where + ": " + std::string(nc_strerror(status)));
    }
  }

  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  void validateFileExists(const std::string& filepath) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("Test failed: Output NetCDF file was not created: " + filepath);
    }
  }

  std::vector<int> readIntVariable(const int ncid, const std::string& name, const std::size_t length) {
    int variableId = -1;
    checkNc(nc_inq_varid(ncid, name.c_str(), &variableId), "inq var " + name);
    std::vector<int>  values(length, -1);
    const std::size_t start[1] = {0};
    const std::size_t count[1] = {length};
    checkNc(nc_get_vara_int(ncid, variableId, start, count, values.data()), "read " + name);
    return values;
  }

  bool isIndexSequence(const std::vector<int>& values) {
    for (std::size_t index = 0; index < values.size(); index++) {
      if (values[index] != static_cast<int>(index)) {
        return false;
      }
    }
    return true;
  }

  void validate3DHexMetadata(const std::string& filepath) {
    validateFileExists(filepath);

    int ncid = -1;
    checkNc(nc_open(filepath.c_str(), NC_NOWRITE, &ncid), "nc_open(" + filepath + ")");

    // Every value is read before the file closes, so a failed expectation never leaves
    // the file handle open.
    auto readVarId = [&](const char* name) {
      int varId = -1;
      checkNc(nc_inq_varid(ncid, name, &varId), std::string("inq var ") + name);
      return varId;
    };
    auto readDimensionLength = [&](const char* name) {
      int dimId = -1;
      checkNc(nc_inq_dimid(ncid, name, &dimId), std::string("inq dim ") + name);
      std::size_t length = 0;
      checkNc(nc_inq_dimlen(ncid, dimId, &length), std::string("dimlen ") + name);
      return length;
    };

    const int meshVar           = readVarId("mesh3d");
    int       topologyDimension = 0;
    checkNc(nc_get_att_int(ncid, meshVar, "topology_dimension", &topologyDimension), "mesh3d topology_dimension");

    const std::size_t      nodeCount     = readDimensionLength("nMesh3d_node");
    const std::size_t      volumeCount   = readDimensionLength("nMesh3d_volume");
    const std::vector<int> nodeIndices   = readIntVariable(ncid, "nMesh3d_node", nodeCount);
    const std::vector<int> volumeIndices = readIntVariable(ncid, "nMesh3d_volume", volumeCount);

    // The MPI scan gives rank 0 the offset 0, so the checks of the first volume pass for every
    // rank count.
    const int   volumeNodesVar       = readVarId("volume_nodes");
    std::size_t connectivityStart[2] = {0, 0};
    std::size_t connectivityCount[2] = {1, 8};
    int         connectivity[8]      = {-2, -2, -2, -2, -2, -2, -2, -2};
    checkNc(
      nc_get_vara_int(ncid, volumeNodesVar, connectivityStart, connectivityCount, connectivity),
      "read volume_nodes"
    );

    std::size_t orderingLen = 0;
    checkNc(nc_inq_attlen(ncid, volumeNodesVar, "node_ordering", &orderingLen), "volume_nodes node_ordering len");
    std::string nodeOrdering(orderingLen, '\0');
    checkNc(nc_get_att_text(ncid, volumeNodesVar, "node_ordering", nodeOrdering.data()), "volume_nodes node_ordering");

    const int   shapeVar      = readVarId("volume_shape_type");
    std::size_t shapeStart[1] = {0};
    std::size_t shapeCount[1] = {1};
    int         shapeTag      = -1;
    checkNc(nc_get_vara_int(ncid, shapeVar, shapeStart, shapeCount, &shapeTag), "read volume_shape_type");

    const int   zVar            = readVarId("z");
    std::size_t zStart[1]       = {0};
    std::size_t zCount[1]       = {8};
    float       zCoordinates[8] = {0.0F};
    checkNc(nc_get_vara_float(ncid, zVar, zStart, zCount, zCoordinates), "read z");

    const int   densityVar = readVarId("Density");
    std::size_t attrLen    = 0;
    checkNc(nc_inq_attlen(ncid, densityVar, "location", &attrLen), "Density location len");
    std::string location(attrLen, '\0');
    checkNc(nc_get_att_text(ncid, densityVar, "location", location.data()), "Density location");
    std::size_t densityStart[1] = {0};
    std::size_t densityCount[1] = {1};
    float       density         = 0.0F;
    checkNc(nc_get_vara_float(ncid, densityVar, densityStart, densityCount, &density), "read Density");

    checkNc(nc_close(ncid), "nc_close(" + filepath + ")");

    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    if (topologyDimension != 3) {
      throw std::runtime_error("Expected NetCDF topology_dimension 3 for the 3D hexahedron test.");
    }
    if (nodeCount != 8 * static_cast<std::size_t>(ranks) or volumeCount != static_cast<std::size_t>(ranks)) {
      throw std::runtime_error("Unexpected global node/volume count for the 3D hexahedron test.");
    }
    if (not isIndexSequence(nodeIndices) or not isIndexSequence(volumeIndices)) {
      throw std::runtime_error("Expected the node and volume indices in nMesh3d_node and nMesh3d_volume.");
    }
    const int expectedConnectivity[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < 8; ++i) {
      if (connectivity[i] != expectedConnectivity[i]) {
        throw std::runtime_error("Unexpected 3D hexahedron connectivity in volume_nodes.");
      }
    }
    if (nodeOrdering.find("VTK_HEXAHEDRON") == std::string::npos) {
      throw std::runtime_error("Expected a documented VTK_HEXAHEDRON node_ordering attribute on volume_nodes.");
    }
    if (shapeTag != 2) {
      throw std::runtime_error("Expected UGRID hexahedron shape tag 2.");
    }
    const float expectedZ[8] = {0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    for (int i = 0; i < 8; ++i) {
      if (zCoordinates[i] != expectedZ[i]) {
        throw std::runtime_error("Unexpected 3D hexahedron z coordinates.");
      }
    }
    if (location != "volume") {
      throw std::runtime_error("Expected 3D cell data location 'volume'.");
    }
    if (density != 2.5F) {
      throw std::runtime_error("Expected 3D cell field Density value 2.5.");
    }
  }

  void validateMultipleSubdomains2D(const std::string& filepath, int ranks) {
    validateFileExists(filepath);

    int ncid = -1;
    checkNc(nc_open(filepath.c_str(), NC_NOWRITE, &ncid), "nc_open(" + filepath + ")");

    int dimNodes = -1;
    int dimFaces = -1;
    checkNc(nc_inq_dimid(ncid, "nMesh2d_node", &dimNodes), "inq nMesh2d_node");
    checkNc(nc_inq_dimid(ncid, "nMesh2d_face", &dimFaces), "inq nMesh2d_face");

    std::size_t nodeCount = 0;
    std::size_t faceCount = 0;
    checkNc(nc_inq_dimlen(ncid, dimNodes, &nodeCount), "len nMesh2d_node");
    checkNc(nc_inq_dimlen(ncid, dimFaces, &faceCount), "len nMesh2d_face");
    if (nodeCount != 7 * static_cast<std::size_t>(ranks) or faceCount != 2 * static_cast<std::size_t>(ranks)) {
      nc_close(ncid);
      throw std::runtime_error("Unexpected global node/face count for multi-subdomain NetCDF test.");
    }

    const std::vector<int> nodeIndices = readIntVariable(ncid, "nMesh2d_node", nodeCount);
    const std::vector<int> faceIndices = readIntVariable(ncid, "nMesh2d_face", faceCount);
    if (not isIndexSequence(nodeIndices) or not isIndexSequence(faceIndices)) {
      nc_close(ncid);
      throw std::runtime_error("Expected the node and face indices in nMesh2d_node and nMesh2d_face.");
    }

    int faceNodesVar = -1;
    checkNc(nc_inq_varid(ncid, "face_nodes", &faceNodesVar), "inq face_nodes");
    std::size_t start[2]     = {0, 0};
    std::size_t count[2]     = {2, 4};
    int         faceNodes[8] = {-2, -2, -2, -2, -2, -2, -2, -2};
    checkNc(nc_get_vara_int(ncid, faceNodesVar, start, count, faceNodes), "read face_nodes");

    const int expectedFaceNodes[8] = {0, 1, 2, 3, 4, 5, 6, -1};
    for (int i = 0; i < 8; ++i) {
      if (faceNodes[i] != expectedFaceNodes[i]) {
        nc_close(ncid);
        throw std::runtime_error("Unexpected multi-subdomain NetCDF connectivity.");
      }
    }

    int materialVar = -1;
    checkNc(nc_inq_varid(ncid, "Material", &materialVar), "inq Material");
    std::size_t fieldStart[1] = {0};
    std::size_t fieldCount[1] = {2};
    float       material[2]   = {0.0F, 0.0F};
    checkNc(nc_get_vara_float(ncid, materialVar, fieldStart, fieldCount, material), "read Material");
    if (material[0] != 3.0F or material[1] != 7.0F) {
      nc_close(ncid);
      throw std::runtime_error("Unexpected multi-subdomain NetCDF cell field values.");
    }

    checkNc(nc_close(ncid), "nc_close(" + filepath + ")");
  }

  void cleanup(const std::string& filename) {
    const std::filesystem::path filePath(filename);
    if (std::filesystem::exists(filePath)) {
      std::filesystem::remove(filePath);
    }
  }
} // namespace

tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void
tarch::plotter::tests::NetCDFFileWriterTest::testHeterogeneousRanks() {
  // Only the global master plots data, and every rank calls writeToFile().
  const std::string filename = "NetCDFTestHeterogeneous";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      auto vertexPlotter   = plotter.createVertexPlotter(0);
      auto cellPlotter     = plotter.createCellPlotter(0);
      auto cellDataPlotter = plotter.createCellDataPlotter(0, "Material", 1);

      const int p0      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      const int p1      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      const int p2      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      const int p3      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      int       quad[4] = {p0, p1, p2, p3};
      const int cell    = cellPlotter->plotQuadrangle(quad);
      cellDataPlotter->plotCellData(cell, 3.0);
    }

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::run() {
  // The read-back helpers report a malformed file by throwing, and the test framework
  // does not guard run(), so this catch records the failure and keeps the binary alive.
  try {
    testMethod(test2DSimpleTriangle);
    testMethod(test2DQuadWithData);
    testMethod(test3DHexahedronWithData);
    testMethod(testMultipleSubdomains2DWithData);
    testMethod(testHeterogeneousRanks);
    testMethod(testUnevenSubdomainCounts);
    testMethod(testEmptyPlotter);
    testMethod(testInconsistentDataSizeThrows);
    testMethod(testUnsupportedCellType);
    testMethod(testInvalidIdentifierThrows);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testInvalidIdentifierThrows() {
  NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  bool thrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  if (not thrown) {
    throw std::runtime_error("Expected the illegal data identifier 'bad/name' to be rejected.");
  }
}

void
tarch::plotter::tests::NetCDFFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "NetCDFTestTriangle";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter = plotter.createVertexPlotter(0);
    auto cellPlotter   = plotter.createCellPlotter(0);

    const int i0         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    const int i1         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    const int i2         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.5, 1.0});
    int       indices[3] = {i0, i1, i2};

    cellPlotter->plotTriangle(indices);

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::test2DQuadWithData() {
  const std::string filename = "NetCDFTestQuad";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter     = plotter.createVertexPlotter(0);
    auto cellPlotter       = plotter.createCellPlotter(0);
    auto vertexDataPlotter = plotter.createVertexDataPlotter(0, "Pressure", 1);
    auto cellDataPlotter   = plotter.createCellDataPlotter(0, "Velocity", 2);

    const int i0         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    const int i1         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    const int i2         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    const int i3         = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int       indices[4] = {i0, i1, i2, i3};

    const int cellIndex = cellPlotter->plotQuadrangle(indices);

    vertexDataPlotter->plotVertexData(i0, 10.0);
    vertexDataPlotter->plotVertexData(i1, 11.0);
    vertexDataPlotter->plotVertexData(i2, 12.0);
    vertexDataPlotter->plotVertexData(i3, 13.0);

    cellDataPlotter->plotCellData(cellIndex, tarch::la::Vector<2, double>{1.5, -1.5});

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::test3DHexahedronWithData() {
  const std::string filename = "NetCDFTestHex";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter   = plotter.createVertexPlotter(0);
    auto cellPlotter     = plotter.createCellPlotter(0);
    auto cellDataPlotter = plotter.createCellDataPlotter(0, "Density", 1);

    int indices[8];
    indices[0] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 0.0});
    indices[1] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 0.0});
    indices[2] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 0.0});
    indices[3] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 0.0});
    indices[4] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 1.0});
    indices[5] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 1.0});
    indices[6] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 1.0});
    indices[7] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 1.0});

    const int cellIndex = cellPlotter->plotHexahedron(indices);
    cellDataPlotter->plotCellData(cellIndex, 2.5);

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validate3DHexMetadata(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testMultipleSubdomains2DWithData() {
  const std::string filename = "NetCDFTestMultipleSubdomains";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    plotter.acquirePiece(1);

    {
      auto vertexPlotter     = plotter.createVertexPlotter(0);
      auto cellPlotter       = plotter.createCellPlotter(0);
      auto vertexDataPlotter = plotter.createVertexDataPlotter(0, "Pressure", 1);
      auto cellDataPlotter   = plotter.createCellDataPlotter(0, "Material", 1);

      const int p0      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      const int p1      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      const int p2      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      const int p3      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      int       quad[4] = {p0, p1, p2, p3};
      const int cell    = cellPlotter->plotQuadrangle(quad);

      vertexDataPlotter->plotVertexData(p0, 10.0);
      vertexDataPlotter->plotVertexData(p1, 11.0);
      vertexDataPlotter->plotVertexData(p2, 12.0);
      vertexDataPlotter->plotVertexData(p3, 13.0);
      cellDataPlotter->plotCellData(cell, 3.0);
    }

    {
      auto vertexPlotter     = plotter.createVertexPlotter(1);
      auto cellPlotter       = plotter.createCellPlotter(1);
      auto vertexDataPlotter = plotter.createVertexDataPlotter(1, "Pressure", 1);
      auto cellDataPlotter   = plotter.createCellDataPlotter(1, "Material", 1);

      const int p0          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
      const int p1          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
      const int p2          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.5, 1.0});
      int       triangle[3] = {p0, p1, p2};
      const int cell        = cellPlotter->plotTriangle(triangle);

      vertexDataPlotter->plotVertexData(p0, 20.0);
      vertexDataPlotter->plotVertexData(p1, 21.0);
      vertexDataPlotter->plotVertexData(p2, 22.0);
      cellDataPlotter->plotCellData(cell, 7.0);
    }

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateMultipleSubdomains2D(fullPath, tarch::mpi::Rank::getInstance().getNumberOfRanks());
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "NetCDFTestUneven";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  // The master plots three subdomains and every other rank two.
  const int numberOfSubdomains = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    for (int subdomain = 0; subdomain < numberOfSubdomains; ++subdomain) {
      plotter.acquirePiece(subdomain);
    }
    for (int subdomain = 0; subdomain < numberOfSubdomains; ++subdomain) {
      auto         vertexPlotter   = plotter.createVertexPlotter(subdomain);
      auto         cellPlotter     = plotter.createCellPlotter(subdomain);
      auto         cellDataPlotter = plotter.createCellDataPlotter(subdomain, "Material", 1);
      const double origin          = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * subdomain;
      const int    p0              = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 0.0, 0.0});
      const int    p1              = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 1.0, 0.0});
      const int    p2              = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 1.0, 1.0});
      const int    p3              = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 0.0, 1.0});
      int          quad[4]         = {p0, p1, p2, p3};
      const int    cell            = cellPlotter->plotQuadrangle(quad);
      cellDataPlotter->plotCellData(cell, 1.0);
    }
    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    const int         ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const std::size_t faces = 3 + 2 * static_cast<std::size_t>(ranks - 1);
    const std::size_t nodes = 4 * faces;

    int ncid = -1;
    checkNc(nc_open(fullPath.c_str(), NC_NOWRITE, &ncid), "nc_open(" + fullPath + ")");
    int dimNodes = -1;
    int dimFaces = -1;
    checkNc(nc_inq_dimid(ncid, "nMesh2d_node", &dimNodes), "inq nMesh2d_node");
    checkNc(nc_inq_dimid(ncid, "nMesh2d_face", &dimFaces), "inq nMesh2d_face");
    std::size_t nodeCount = 0;
    std::size_t faceCount = 0;
    checkNc(nc_inq_dimlen(ncid, dimNodes, &nodeCount), "len nMesh2d_node");
    checkNc(nc_inq_dimlen(ncid, dimFaces, &faceCount), "len nMesh2d_face");
    const std::vector<int> nodeIndices = readIntVariable(ncid, "nMesh2d_node", nodeCount);
    const std::vector<int> faceIndices = readIntVariable(ncid, "nMesh2d_face", faceCount);
    checkNc(nc_close(ncid), "nc_close(" + fullPath + ")");
    if (nodeCount != nodes or faceCount != faces) {
      throw std::runtime_error("Unexpected merged node/face count for the uneven-subdomain NetCDF test.");
    }
    if (not isIndexSequence(nodeIndices) or not isIndexSequence(faceIndices)) {
      throw std::runtime_error("Expected the node and face indices in nMesh2d_node and nMesh2d_face.");
    }
    cleanup(fullPath);
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testEmptyPlotter() {
  const std::string filename = "NetCDFTestEmpty";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    if (std::filesystem::exists(fullPath)) {
      cleanup(fullPath);
    }
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "NetCDFTestInconsistent";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter     = plotter.createVertexPlotter(0);
    auto vertexDataPlotter = plotter.createVertexDataPlotter(0, "BrokenField", 1);

    vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});

    // The write validation rejects a field with a value for one of the two vertices only.
    vertexDataPlotter->plotVertexData(0, 5.0);

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  } catch (...) {
    exceptionThrown = true;
  }

  barrier();
  if (not exceptionThrown) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup(fullPath);
    }
    throw std::runtime_error("NetCDFFileWriter did not throw on inconsistent data size.");
  } else {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup(fullPath);
    }
  }
  barrier();
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testUnsupportedCellType() {
  const std::string filename = "NetCDFTestUnsupported";

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string());
    plotter.acquirePiece(0);
    auto vertexPlotter = plotter.createVertexPlotter(0);
    auto cellPlotter   = plotter.createCellPlotter(0);

    int indices[2];
    indices[0] = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    indices[1] = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});

    cellPlotter->plotLine(indices);

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  barrier();
  if (not exceptionThrown) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup(filename + ".nc");
    }
    throw std::runtime_error("NetCDFFileWriter did not throw on the unsupported cell type Line.");
  }
  barrier();
}

#else // PEANO_WITH_NETCDF

// A build without NetCDF runs an empty test.
tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void
tarch::plotter::tests::NetCDFFileWriterTest::run() {}

#endif
