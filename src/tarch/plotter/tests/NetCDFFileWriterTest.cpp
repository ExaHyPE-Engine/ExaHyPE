// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/plotter/NetCDFFileWriter.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>

namespace {
  void checkNc(int status, const std::string& where) {
    if (status != NC_NOERR) {
      throw std::runtime_error("NetCDF test error in " + where + ": " + std::string(nc_strerror(status)));
    }
  }

  void barrier() {
#ifdef PEANO_WITH_MPI
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
  }

  void validateFileExists(const std::string& filepath) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("Test failed: Output NetCDF file was not created: " + filepath);
    }
  }

  void validate3DHexMetadata(const std::string& filepath) {
    validateFileExists(filepath);

    int ncid = -1;
    checkNc(nc_open(filepath.c_str(), NC_NOWRITE, &ncid), "nc_open(" + filepath + ")");

    // Read everything we assert first, close the file, then check, so a failed
    // expectation never leaves the file handle open.
    auto readVarId = [&](const char* name) {
      int varId = -1;
      checkNc(nc_inq_varid(ncid, name, &varId), std::string("inq var ") + name);
      return varId;
    };
    auto readDimLen = [&](const char* name) {
      int dimId = -1;
      checkNc(nc_inq_dimid(ncid, name, &dimId), std::string("inq dim ") + name);
      std::size_t length = 0;
      checkNc(nc_inq_dimlen(ncid, dimId, &length), std::string("dimlen ") + name);
      return length;
    };

    const int meshVar           = readVarId("mesh3d");
    int       topologyDimension = 0;
    checkNc(nc_get_att_int(ncid, meshVar, "topology_dimension", &topologyDimension), "mesh3d topology_dimension");

    const std::size_t nodeCount   = readDimLen("nMesh3d_node");
    const std::size_t volumeCount = readDimLen("nMesh3d_volume");

    // The first volume belongs to rank 0 (offsets are assigned by an MPI scan,
    // so rank 0 starts at 0), so its connectivity and node coordinates are
    // deterministic regardless of the number of ranks.
    const int   volumeNodesVar  = readVarId("volume_nodes");
    std::size_t connStart[2]    = {0, 0};
    std::size_t connCount[2]    = {1, 8};
    int         connectivity[8] = {-2, -2, -2, -2, -2, -2, -2, -2};
    checkNc(nc_get_vara_int(ncid, volumeNodesVar, connStart, connCount, connectivity), "read volume_nodes");

    std::size_t orderingLen = 0;
    checkNc(nc_inq_attlen(ncid, volumeNodesVar, "node_ordering", &orderingLen), "volume_nodes node_ordering len");
    std::string nodeOrdering(orderingLen, '\0');
    checkNc(nc_get_att_text(ncid, volumeNodesVar, "node_ordering", nodeOrdering.data()), "volume_nodes node_ordering");

    const int   shapeVar      = readVarId("volume_shape_type");
    std::size_t shapeStart[1] = {0};
    std::size_t shapeCount[1] = {1};
    int         shape         = -1;
    checkNc(nc_get_vara_int(ncid, shapeVar, shapeStart, shapeCount, &shape), "read volume_shape_type");

    const int   zVar      = readVarId("z");
    std::size_t zStart[1] = {0};
    std::size_t zCount[1] = {8};
    float       z[8]      = {0.0f};
    checkNc(nc_get_vara_float(ncid, zVar, zStart, zCount, z), "read z");

    const int   densityVar = readVarId("Density");
    std::size_t attrLen    = 0;
    checkNc(nc_inq_attlen(ncid, densityVar, "location", &attrLen), "Density location len");
    std::string location(attrLen, '\0');
    checkNc(nc_get_att_text(ncid, densityVar, "location", location.data()), "Density location");
    std::size_t densityStart[1] = {0};
    std::size_t densityCount[1] = {1};
    float       density         = 0.0f;
    checkNc(nc_get_vara_float(ncid, densityVar, densityStart, densityCount, &density), "read Density");

    checkNc(nc_close(ncid), "nc_close(" + filepath + ")");

    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    if (topologyDimension != 3) {
      throw std::runtime_error("Expected NetCDF topology_dimension 3 for the 3D hexahedron test.");
    }
    if (nodeCount != static_cast<std::size_t>(8 * ranks) or volumeCount != static_cast<std::size_t>(ranks)) {
      throw std::runtime_error("Unexpected global node/volume count for the 3D hexahedron test.");
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
    if (shape != 2) {
      throw std::runtime_error("Expected UGRID hexahedron shape tag 2.");
    }
    const float expectedZ[8] = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    for (int i = 0; i < 8; ++i) {
      if (z[i] != expectedZ[i]) {
        throw std::runtime_error("Unexpected 3D hexahedron z coordinates.");
      }
    }
    if (location != "volume") {
      throw std::runtime_error("Expected 3D cell data location 'volume'.");
    }
    if (density != 2.5f) {
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
    if (nodeCount != static_cast<std::size_t>(7 * ranks) or faceCount != static_cast<std::size_t>(2 * ranks)) {
      nc_close(ncid);
      throw std::runtime_error("Unexpected global node/face count for multi-subdomain NetCDF test.");
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
    float       material[2]   = {0.0f, 0.0f};
    checkNc(nc_get_vara_float(ncid, materialVar, fieldStart, fieldCount, material), "read Material");
    if (material[0] != 3.0f or material[1] != 7.0f) {
      nc_close(ncid);
      throw std::runtime_error("Unexpected multi-subdomain NetCDF cell field values.");
    }

    checkNc(nc_close(ncid), "nc_close(" + filepath + ")");
  }

  void cleanup(const std::string& filename) {
    std::filesystem::path p(filename);
    if (std::filesystem::exists(p)) {
      std::filesystem::remove(p);
    }
  }
} // namespace

tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void tarch::plotter::tests::NetCDFFileWriterTest::testHeterogeneousRanks() {
  // Only the global master contributes geometry + a "Material" cell field; every
  // other rank leaves the plotter empty. With parallel collective IO the empty
  // ranks must still define and write the same NetCDF variables, which only works
  // because the field schema is reduced globally. Without that, this writeToFile
  // deadlocks (or corrupts the file) for more than one rank.
  const std::string filename = "NetCDFTestHeterogeneous";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      auto v  = plotter.createVertexPlotter(0);
      auto c  = plotter.createCellPlotter(0);
      auto cd = plotter.createCellDataPlotter(0, "Material", 1);

      int p0      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      int p1      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      int p2      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      int p3      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      int quad[4] = {p0, p1, p2, p3};
      int cell    = c->plotQuadrangle(quad);
      cd->plotCellData(cell, 3.0);
    }
    // All non-master ranks intentionally contribute nothing.

    plotter.writeToFile(filename, 0); // collective; must not deadlock
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::run() {
  // The read-back helpers report a malformed file by throwing. Without this catch the
  // exception leaves run(), which the test framework does not guard, and the whole unit-test
  // binary aborts instead of recording a failed case. The remaining cases are then skipped,
  // so the counter has to be raised here as well.
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

void tarch::plotter::tests::NetCDFFileWriterTest::testInvalidIdentifierThrows() {
  NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  bool             thrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  if (not thrown) {
    throw std::runtime_error("Expected the illegal data identifier 'bad/name' to be rejected.");
  }
}

void tarch::plotter::tests::NetCDFFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "NetCDFTestTriangle";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v = plotter.createVertexPlotter(0);
    auto             c = plotter.createCellPlotter(0);

    int i0         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int i1         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int i2         = v->plotVertex(tarch::la::Vector<2, double>{0.5, 1.0});
    int indices[3] = {i0, i1, i2};

    c->plotTriangle(indices);

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::test2DQuadWithData() {
  const std::string filename = "NetCDFTestQuad";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             c  = plotter.createCellPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "Pressure", 1);
    auto             cd = plotter.createCellDataPlotter(0, "Velocity", 2);

    int i0         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int i1         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int i2         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    int i3         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int indices[4] = {i0, i1, i2, i3};

    int cellIndex = c->plotQuadrangle(indices);

    // Add Data
    vd->plotVertexData(i0, 10.0);
    vd->plotVertexData(i1, 11.0);
    vd->plotVertexData(i2, 12.0);
    vd->plotVertexData(i3, 13.0);

    cd->plotCellData(cellIndex, tarch::la::Vector<2, double>{1.5, -1.5});

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::test3DHexahedronWithData() {
  const std::string filename = "NetCDFTestHex";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             c  = plotter.createCellPlotter(0);
    auto             cd = plotter.createCellDataPlotter(0, "Density", 1);

    int indices[8];
    indices[0] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 0.0});
    indices[1] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 0.0});
    indices[2] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 0.0});
    indices[3] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 0.0});
    indices[4] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 1.0});
    indices[5] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 1.0});
    indices[6] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 1.0});
    indices[7] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 1.0});

    const int cellIndex = c->plotHexahedron(indices);
    cd->plotCellData(cellIndex, 2.5);

    plotter.writeToFile(filename, 0);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validate3DHexMetadata(fullPath);
    cleanup(fullPath);
  }
  barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::testMultipleSubdomains2DWithData() {
  const std::string filename = "NetCDFTestMultipleSubdomains";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 2);

    {
      auto v  = plotter.createVertexPlotter(0);
      auto c  = plotter.createCellPlotter(0);
      auto vd = plotter.createVertexDataPlotter(0, "Pressure", 1);
      auto cd = plotter.createCellDataPlotter(0, "Material", 1);

      int p0      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      int p1      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      int p2      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      int p3      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      int quad[4] = {p0, p1, p2, p3};
      int cell    = c->plotQuadrangle(quad);

      vd->plotVertexData(p0, 10.0);
      vd->plotVertexData(p1, 11.0);
      vd->plotVertexData(p2, 12.0);
      vd->plotVertexData(p3, 13.0);
      cd->plotCellData(cell, 3.0);
    }

    {
      auto v  = plotter.createVertexPlotter(1);
      auto c  = plotter.createCellPlotter(1);
      auto vd = plotter.createVertexDataPlotter(1, "Pressure", 1);
      auto cd = plotter.createCellDataPlotter(1, "Material", 1);

      int p0          = v->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
      int p1          = v->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
      int p2          = v->plotVertex(tarch::la::Vector<2, double>{2.5, 1.0});
      int triangle[3] = {p0, p1, p2};
      int cell        = c->plotTriangle(triangle);

      vd->plotVertexData(p0, 20.0);
      vd->plotVertexData(p1, 21.0);
      vd->plotVertexData(p2, 22.0);
      cd->plotCellData(cell, 7.0);
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

void tarch::plotter::tests::NetCDFFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "NetCDFTestUneven";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  // Master holds 3 subdomains, every other rank 2 - non-zero but unequal counts.
  const int numberOfSubdomains = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), numberOfSubdomains);
    for (int sd = 0; sd < numberOfSubdomains; ++sd) {
      auto         v       = plotter.createVertexPlotter(sd);
      auto         c       = plotter.createCellPlotter(sd);
      auto         cd      = plotter.createCellDataPlotter(sd, "Material", 1);
      const double s       = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * sd;
      int          p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
      int          p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
      int          p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
      int          p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
      int          quad[4] = {p0, p1, p2, p3};
      int          cell    = c->plotQuadrangle(quad);
      cd->plotCellData(cell, 1.0);
    }
    plotter.writeToFile(filename, 0); // uneven non-zero counts; must not deadlock
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    const int         ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const std::size_t faces = static_cast<std::size_t>(3 + 2 * (ranks - 1)); // master 3, others 2
    const std::size_t nodes = 4 * faces;

    int ncid = -1;
    checkNc(nc_open(fullPath.c_str(), NC_NOWRITE, &ncid), "nc_open(" + fullPath + ")");
    int dimNodes = -1, dimFaces = -1;
    checkNc(nc_inq_dimid(ncid, "nMesh2d_node", &dimNodes), "inq nMesh2d_node");
    checkNc(nc_inq_dimid(ncid, "nMesh2d_face", &dimFaces), "inq nMesh2d_face");
    std::size_t nodeCount = 0, faceCount = 0;
    checkNc(nc_inq_dimlen(ncid, dimNodes, &nodeCount), "len nMesh2d_node");
    checkNc(nc_inq_dimlen(ncid, dimFaces, &faceCount), "len nMesh2d_face");
    checkNc(nc_close(ncid), "nc_close(" + fullPath + ")");
    if (nodeCount != nodes or faceCount != faces) {
      throw std::runtime_error("Unexpected merged node/face count for the uneven-subdomain NetCDF test.");
    }
    cleanup(fullPath);
  }
  barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::testEmptyPlotter() {
  const std::string filename = "NetCDFTestEmpty";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
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

void tarch::plotter::tests::NetCDFFileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "NetCDFTestInconsistent";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "BrokenField", 1);

    v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});

    // Data for only one of the two vertices, which the write validation rejects.
    vd->plotVertexData(0, 5.0);

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

void tarch::plotter::tests::NetCDFFileWriterTest::testUnsupportedCellType() {
  const std::string filename = "NetCDFTestUnsupported";

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v = plotter.createVertexPlotter(0);
    auto             c = plotter.createCellPlotter(0);

    int indices[2];
    indices[0] = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    indices[1] = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});

    c->plotLine(indices);

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

// Dummy implementation for the build when NetCDF is disabled.
tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void tarch::plotter::tests::NetCDFFileWriterTest::run() {}

#endif
