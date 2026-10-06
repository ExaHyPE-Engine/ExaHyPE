// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriterTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/NetCDFFileWriter.h"
#include "tarch/plotter/tests/Meshes.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>

namespace {
  void checkNc(const int status, const std::string& subject) {
    if (status != NC_NOERR) {
      throw std::runtime_error("NetCDF reports \"" + std::string(nc_strerror(status)) + "\" for " + subject + ".");
    }
  }

  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  std::vector<int> readIntVariable(const int ncid, const std::string& name, const std::size_t length) {
    int variableId = -1;
    checkNc(nc_inq_varid(ncid, name.c_str(), &variableId), "the variable " + name);
    std::vector<int>  values(length, -1);
    const std::size_t start[1] = {0};
    const std::size_t count[1] = {length};
    checkNc(nc_get_vara_int(ncid, variableId, start, count, values.data()), "the variable " + name);
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

  void validateFileExists(const std::string& filepath) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("The test expects the file '" + filepath + "'.");
    }
  }

  void validate3DHexahedronFile(const std::string& filepath) {
    validateFileExists(filepath);

    int ncid = -1;
    checkNc(nc_open(filepath.c_str(), NC_NOWRITE, &ncid), "the file " + filepath);

    // The test reads every value before it closes the file.
    const auto readVarId = [&](const char* const name) {
      int varId = -1;
      checkNc(nc_inq_varid(ncid, name, &varId), std::string("the variable ") + name);
      return varId;
    };
    const auto readDimensionLength = [&](const char* const name) {
      int dimId = -1;
      checkNc(nc_inq_dimid(ncid, name, &dimId), std::string("the dimension ") + name);
      std::size_t length = 0;
      checkNc(nc_inq_dimlen(ncid, dimId, &length), std::string("the dimension ") + name);
      return length;
    };

    const int meshVar           = readVarId("mesh3d");
    int       topologyDimension = 0;
    checkNc(
      nc_get_att_int(ncid, meshVar, "topology_dimension", &topologyDimension),
      "the attribute topology_dimension"
    );

    const std::size_t      nodeCount     = readDimensionLength("nMesh3d_node");
    const std::size_t      volumeCount   = readDimensionLength("nMesh3d_volume");
    const std::vector<int> nodeIndices   = readIntVariable(ncid, "nMesh3d_node", nodeCount);
    const std::vector<int> volumeIndices = readIntVariable(ncid, "nMesh3d_volume", volumeCount);

    // The first volume of the file belongs to rank 0 for every rank count.
    const int         volumeNodesVar       = readVarId("volume_nodes");
    const std::size_t connectivityStart[2] = {0, 0};
    const std::size_t connectivityCount[2] = {1, 8};
    int               connectivity[8]      = {-2, -2, -2, -2, -2, -2, -2, -2};
    checkNc(
      nc_get_vara_int(ncid, volumeNodesVar, connectivityStart, connectivityCount, connectivity),
      "the variable volume_nodes"
    );

    std::size_t orderingLength = 0;
    checkNc(nc_inq_attlen(ncid, volumeNodesVar, "node_ordering", &orderingLength), "the attribute node_ordering");
    std::string nodeOrdering(orderingLength, '\0');
    checkNc(nc_get_att_text(ncid, volumeNodesVar, "node_ordering", nodeOrdering.data()), "the attribute node_ordering");

    const int         shapeVar      = readVarId("volume_shape_type");
    const std::size_t shapeStart[1] = {0};
    const std::size_t shapeCount[1] = {1};
    int               shapeTag      = -1;
    checkNc(nc_get_vara_int(ncid, shapeVar, shapeStart, shapeCount, &shapeTag), "the variable volume_shape_type");

    const int         zVar            = readVarId("z");
    const std::size_t zStart[1]       = {0};
    const std::size_t zCount[1]       = {8};
    float             zCoordinates[8] = {0.0F};
    checkNc(nc_get_vara_float(ncid, zVar, zStart, zCount, zCoordinates), "the variable z");

    const int   densityVar     = readVarId("Density");
    std::size_t locationLength = 0;
    checkNc(nc_inq_attlen(ncid, densityVar, "location", &locationLength), "the attribute location");
    std::string location(locationLength, '\0');
    checkNc(nc_get_att_text(ncid, densityVar, "location", location.data()), "the attribute location");
    const std::size_t densityStart[1] = {0};
    const std::size_t densityCount[1] = {1};
    float             density         = 0.0F;
    checkNc(nc_get_vara_float(ncid, densityVar, densityStart, densityCount, &density), "the variable Density");

    checkNc(nc_close(ncid), "the file " + filepath);

    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    if (topologyDimension != 3) {
      throw std::runtime_error("The test expects 3 in the attribute topology_dimension.");
    }
    if (nodeCount != 8 * static_cast<std::size_t>(ranks) or volumeCount != static_cast<std::size_t>(ranks)) {
      throw std::runtime_error("The test expects eight nodes and one volume per rank in the file.");
    }
    if (not isIndexSequence(nodeIndices) or not isIndexSequence(volumeIndices)) {
      throw std::runtime_error("The test expects the node and volume indices in nMesh3d_node and nMesh3d_volume.");
    }
    const int expectedConnectivity[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int node = 0; node < 8; node++) {
      if (connectivity[node] != expectedConnectivity[node]) {
        throw std::runtime_error(
          "The test expects the nodes 0 to 7 for the first volume in the variable volume_nodes."
        );
      }
    }
    if (nodeOrdering.find("VTK_HEXAHEDRON") == std::string::npos) {
      throw std::runtime_error("The test expects the corner order of VTK_HEXAHEDRON in the attribute node_ordering.");
    }
    if (shapeTag != 2) {
      throw std::runtime_error("The test expects the hexahedron tag 2 in the variable volume_shape_type.");
    }
    const float expectedZ[8] = {0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    for (int node = 0; node < 8; node++) {
      if (zCoordinates[node] != expectedZ[node]) {
        throw std::runtime_error("The test expects the corner heights of the unit cube in the variable z.");
      }
    }
    if (location != "volume") {
      throw std::runtime_error("The test expects 'volume' in the attribute location of the variable Density.");
    }
    if (density != 2.5F) {
      throw std::runtime_error("The test expects 2.5 in the variable Density.");
    }
  }

  void validateTwoQuadrangles2D(const std::string& filepath, const int ranks) {
    validateFileExists(filepath);

    int ncid = -1;
    checkNc(nc_open(filepath.c_str(), NC_NOWRITE, &ncid), "the file " + filepath);

    int dimNodes = -1;
    int dimFaces = -1;
    checkNc(nc_inq_dimid(ncid, "nMesh2d_node", &dimNodes), "the dimension nMesh2d_node");
    checkNc(nc_inq_dimid(ncid, "nMesh2d_face", &dimFaces), "the dimension nMesh2d_face");

    std::size_t nodeCount = 0;
    std::size_t faceCount = 0;
    checkNc(nc_inq_dimlen(ncid, dimNodes, &nodeCount), "the dimension nMesh2d_node");
    checkNc(nc_inq_dimlen(ncid, dimFaces, &faceCount), "the dimension nMesh2d_face");
    if (nodeCount != 8 * static_cast<std::size_t>(ranks) or faceCount != 2 * static_cast<std::size_t>(ranks)) {
      nc_close(ncid);
      throw std::runtime_error("The test expects eight nodes and two faces per rank in the file.");
    }

    const std::vector<int> nodeIndices = readIntVariable(ncid, "nMesh2d_node", nodeCount);
    const std::vector<int> faceIndices = readIntVariable(ncid, "nMesh2d_face", faceCount);
    if (not isIndexSequence(nodeIndices) or not isIndexSequence(faceIndices)) {
      nc_close(ncid);
      throw std::runtime_error("The test expects the node and face indices in nMesh2d_node and nMesh2d_face.");
    }

    int faceNodesVar = -1;
    checkNc(nc_inq_varid(ncid, "face_nodes", &faceNodesVar), "the variable face_nodes");
    const std::size_t start[2]     = {0, 0};
    const std::size_t count[2]     = {2, 4};
    int               faceNodes[8] = {-2, -2, -2, -2, -2, -2, -2, -2};
    checkNc(nc_get_vara_int(ncid, faceNodesVar, start, count, faceNodes), "the variable face_nodes");

    const int expectedFaceNodes[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int node = 0; node < 8; node++) {
      if (faceNodes[node] != expectedFaceNodes[node]) {
        nc_close(ncid);
        throw std::runtime_error(
          "The test expects the nodes 0 to 7 for the first two faces in the variable face_nodes."
        );
      }
    }

    int materialVar = -1;
    checkNc(nc_inq_varid(ncid, "Material", &materialVar), "the variable Material");
    const std::size_t fieldStart[1] = {0};
    const std::size_t fieldCount[1] = {2};
    float             material[2]   = {0.0F, 0.0F};
    checkNc(nc_get_vara_float(ncid, materialVar, fieldStart, fieldCount, material), "the variable Material");
    if (material[0] != 3.0F or material[1] != 7.0F) {
      nc_close(ncid);
      throw std::runtime_error("The test expects 3 and 7 for the first two faces in the variable Material.");
    }

    checkNc(nc_close(ncid), "the file " + filepath);
  }

  std::string getPath(const std::string& name) { return (tarch::tests::getWritableTestDirectory() / name).string(); }

  /**
   * Writes mesh as the snapshot at the time 0 and returns the path of its file.
   */
  std::string writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::NetCDFFileWriter writer;
    writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
    barrier();
    return getPath(filename + "-0_000000.nc");
  }

  void removeSeries(const std::string& filename) {
    barrier();
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove(getPath(filename + "-0_000000.nc"));
      std::filesystem::remove(getPath(filename + ".nc.series"));
    }
    barrier();
  }
} // namespace

tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

/**
 * The collective write completes when only the global master writes cells.
 */
void
tarch::plotter::tests::NetCDFFileWriterTest::testHeterogeneousRanks() {
  const std::string filename = "NetCDFTestHeterogeneous";

  tarch::plotter::Mesh mesh;
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    appendUnitSquares(mesh, 0.0, 1);
    mesh.cellFields.push_back({"Material", 1, {3.0}});
  } else {
    mesh.cellFields.push_back({"Material", 1, {}});
  }
  const std::string path = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(path);
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::NetCDFFileWriterTest::run() {
  // The read-back helpers throw for a malformed file. An exception that leaves run() ends the test
  // executable.
  try {
    testMethod(test2DQuadrangleWithData);
    testMethod(test3DHexahedronWithData);
    testMethod(testTwoQuadrangles2DWithData);
    testMethod(testHeterogeneousRanks);
    testMethod(testUnevenCellCounts);
    testMethod(testEmptyMesh);
    testMethod(testSeries);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

void
tarch::plotter::tests::NetCDFFileWriterTest::test2DQuadrangleWithData() {
  const std::string filename = "NetCDFTestQuadrangle";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  mesh.pointFields.push_back({"Pressure", 1, {10.0, 11.0, 12.0, 13.0}});
  mesh.cellFields.push_back({"Velocity", 2, {1.5, -1.5}});
  const std::string path = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(path);
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::NetCDFFileWriterTest::test3DHexahedronWithData() {
  const std::string filename = "NetCDFTestHexahedron";

  tarch::plotter::Mesh mesh;
  appendUnitCube(mesh, 0.0);
  mesh.cellFields.push_back({"Density", 1, {2.5}});
  const std::string path = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validate3DHexahedronFile(path);
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::NetCDFFileWriterTest::testTwoQuadrangles2DWithData() {
  const std::string filename = "NetCDFTestTwoQuadrangles";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 2);
  mesh.pointFields.push_back({"Pressure", 1, {10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0}});
  mesh.cellFields.push_back({"Material", 1, {3.0, 7.0}});
  const std::string path = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateTwoQuadrangles2D(path, tarch::mpi::Rank::getInstance().getNumberOfRanks());
  }
  removeSeries(filename);
}

/**
 * The collective write completes when the ranks hold different non-zero cell counts.
 */
void
tarch::plotter::tests::NetCDFFileWriterTest::testUnevenCellCounts() {
  const std::string filename = "NetCDFTestUneven";

  tarch::plotter::Mesh mesh;
  const int            numberOfCells = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  appendUnitSquares(mesh, 100.0 * tarch::mpi::Rank::getInstance().getRank(), numberOfCells);
  mesh.cellFields.push_back({"Material", 1, tarch::plotter::MeshArray<double>(numberOfCells, 1.0)});
  const std::string path = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(path);
    const int         ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const std::size_t faces = 3 + 2 * static_cast<std::size_t>(ranks - 1);
    const std::size_t nodes = 4 * faces;

    int ncid = -1;
    checkNc(nc_open(path.c_str(), NC_NOWRITE, &ncid), "the file " + path);
    int dimNodes = -1;
    int dimFaces = -1;
    checkNc(nc_inq_dimid(ncid, "nMesh2d_node", &dimNodes), "the dimension nMesh2d_node");
    checkNc(nc_inq_dimid(ncid, "nMesh2d_face", &dimFaces), "the dimension nMesh2d_face");
    std::size_t nodeCount = 0;
    std::size_t faceCount = 0;
    checkNc(nc_inq_dimlen(ncid, dimNodes, &nodeCount), "the dimension nMesh2d_node");
    checkNc(nc_inq_dimlen(ncid, dimFaces, &faceCount), "the dimension nMesh2d_face");
    checkNc(nc_close(ncid), "the file " + path);
    if (nodeCount != nodes or faceCount != faces) {
      throw std::runtime_error("The test expects the nodes and the faces of all ranks in the file.");
    }
  }
  removeSeries(filename);
}

/**
 * A mesh without points, cells and fields gives a valid file.
 */
void
tarch::plotter::tests::NetCDFFileWriterTest::testEmptyMesh() {
  const std::string filename = "NetCDFTestEmpty";

  const std::string path = writeMesh(tarch::plotter::Mesh(), filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(path);
  }
  removeSeries(filename);
}

/**
 * The .nc.series file lists every snapshot file that one writer writes once.
 */
void
tarch::plotter::tests::NetCDFFileWriterTest::testSeries() {
  const std::string filename = "NetCDFTestSeries";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  tarch::plotter::NetCDFFileWriter writer;
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.5);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.5);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::ifstream stream(getPath(filename + ".nc.series"));
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();
    const std::string lastEntry = R"({ "name" : "NetCDFTestSeries-0_500000.nc", "time" : 0.5 })";
    if (
      content.find(R"({ "name" : "NetCDFTestSeries-0_000000.nc", "time" : 0 },)") == std::string::npos
      or content.find(lastEntry) == std::string::npos or content.find(lastEntry) != content.rfind(lastEntry)
    ) {
      throw std::runtime_error("The test expects both snapshots once in the file NetCDFTestSeries.nc.series.");
    }
    std::filesystem::remove(getPath(filename + "-0_000000.nc"));
    std::filesystem::remove(getPath(filename + "-0_500000.nc"));
    std::filesystem::remove(getPath(filename + ".nc.series"));
  }
  barrier();
}

#else // PEANO_WITH_NETCDF

// A build without NetCDF runs no NetCDF test.
tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void
tarch::plotter::tests::NetCDFFileWriterTest::run() {}

#endif
