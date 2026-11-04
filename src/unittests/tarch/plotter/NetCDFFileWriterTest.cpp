// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/NetCDFFileWriter.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Mesh.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/plotter/Meshes.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  std::string getPath(const std::string& name) { return (tarch::testing::getWritableTestDirectory() / name).string(); }

  /**
   * The status of a NetCDF call that read a file back, with a description of the call.
   */
  struct NetCDFCall final {
    int         status{NC_NOERR};
    std::string description{};
  };

  /**
   * The 3D mesh of one hexahedron per rank as the global master read it back after the collective
   * write, with the status of every call that read it.
   */
  struct HexahedronMesh final {
    bool                    exists{false};
    std::vector<NetCDFCall> calls{};
    int                     topologyDimension{0};
    std::size_t             nodeCount{0};
    std::size_t             volumeCount{0};
    std::vector<int>        nodeIndices{};
    std::vector<int>        volumeIndices{};
    std::array<int, 8>      connectivity{-2, -2, -2, -2, -2, -2, -2, -2};
    std::string             nodeOrdering{};
    int                     shapeTag{-1};
    std::array<float, 8>    zCoordinates{};
    std::string             location{};
    float                   density{0.0F};
  };

  /**
   * The 2D mesh as the global master read it back after the collective write, with the status of
   * every call that read it. The nodes and the Material of the first two faces keep their defaults
   * where a reader skips them.
   */
  struct FaceMesh final {
    bool                    exists{false};
    std::vector<NetCDFCall> calls{};
    std::size_t             nodeCount{0};
    std::size_t             faceCount{0};
    std::vector<int>        nodeIndices{};
    std::vector<int>        faceIndices{};
    std::array<int, 8>      faceNodes{-2, -2, -2, -2, -2, -2, -2, -2};
    std::array<float, 2>    material{};
  };

  /**
   * Returns the integers of the variable name of the open file fileId and records the status of
   * every call in calls.
   */
  std::vector<int> readIntegerVariable(
    const int                fileId,
    const std::string&       name,
    const std::size_t        length,
    std::vector<NetCDFCall>& calls
  ) {
    int variableId = -1;
    calls.push_back(NetCDFCall{nc_inq_varid(fileId, name.c_str(), &variableId), "inq var " + name});
    std::vector<int>  values(length, -1);
    const std::size_t start[1] = {0};
    const std::size_t count[1] = {length};
    calls.push_back(NetCDFCall{nc_get_vara_int(fileId, variableId, start, count, values.data()), "read " + name});
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

  /**
   * Reads the hexahedron mesh of the file and records the status of every call.
   */
  HexahedronMesh readHexahedronMesh(const std::string& filePath) {
    HexahedronMesh mesh{.exists = std::filesystem::exists(filePath)};
    const auto     record = [&mesh](const int status, const std::string& where) {
      mesh.calls.push_back(NetCDFCall{.status = status, .description = where});
    };

    int fileId = -1;
    record(nc_open(filePath.c_str(), NC_NOWRITE, &fileId), "nc_open(" + filePath + ")");

    const auto readVariableId = [&](const char* const name) {
      int variableId = -1;
      record(nc_inq_varid(fileId, name, &variableId), std::string("inq var ") + name);
      return variableId;
    };
    const auto readDimensionLength = [&](const char* const name) {
      int dimensionId = -1;
      record(nc_inq_dimid(fileId, name, &dimensionId), std::string("inq dim ") + name);
      std::size_t length = 0;
      record(nc_inq_dimlen(fileId, dimensionId, &length), std::string("dimlen ") + name);
      return length;
    };

    const int meshVariableId = readVariableId("mesh3d");
    record(
      nc_get_att_int(fileId, meshVariableId, "topology_dimension", &mesh.topologyDimension),
      "mesh3d topology_dimension"
    );

    mesh.nodeCount     = readDimensionLength("nMesh3d_node");
    mesh.volumeCount   = readDimensionLength("nMesh3d_volume");
    mesh.nodeIndices   = readIntegerVariable(fileId, "nMesh3d_node", mesh.nodeCount, mesh.calls);
    mesh.volumeIndices = readIntegerVariable(fileId, "nMesh3d_volume", mesh.volumeCount, mesh.calls);

    // The first volume of the file belongs to rank 0 for every rank count.
    const int         volumeNodesVariableId = readVariableId("volume_nodes");
    const std::size_t connectivityStart[2]  = {0, 0};
    const std::size_t connectivityCount[2]  = {1, 8};
    record(
      nc_get_vara_int(fileId, volumeNodesVariableId, connectivityStart, connectivityCount, mesh.connectivity.data()),
      "read volume_nodes"
    );

    std::size_t orderingLength = 0;
    record(
      nc_inq_attlen(fileId, volumeNodesVariableId, "node_ordering", &orderingLength),
      "volume_nodes node_ordering len"
    );
    mesh.nodeOrdering.assign(orderingLength, '\0');
    record(
      nc_get_att_text(fileId, volumeNodesVariableId, "node_ordering", mesh.nodeOrdering.data()),
      "volume_nodes node_ordering"
    );

    const int         shapeVariableId = readVariableId("volume_shape_type");
    const std::size_t shapeStart[1]   = {0};
    const std::size_t shapeCount[1]   = {1};
    record(nc_get_vara_int(fileId, shapeVariableId, shapeStart, shapeCount, &mesh.shapeTag), "read volume_shape_type");

    const int         zVariableId = readVariableId("z");
    const std::size_t zStart[1]   = {0};
    const std::size_t zCount[1]   = {8};
    record(nc_get_vara_float(fileId, zVariableId, zStart, zCount, mesh.zCoordinates.data()), "read z");

    const int   densityVariableId = readVariableId("Density");
    std::size_t locationLength    = 0;
    record(nc_inq_attlen(fileId, densityVariableId, "location", &locationLength), "Density location len");
    mesh.location.assign(locationLength, '\0');
    record(nc_get_att_text(fileId, densityVariableId, "location", mesh.location.data()), "Density location");
    const std::size_t densityStart[1] = {0};
    const std::size_t densityCount[1] = {1};
    record(nc_get_vara_float(fileId, densityVariableId, densityStart, densityCount, &mesh.density), "read Density");

    record(nc_close(fileId), "nc_close(" + filePath + ")");
    return mesh;
  }

  /**
   * Reads the node and face counts and indices, the nodes of the first two faces and their Material,
   * and records the status of every call.
   */
  FaceMesh readFaceMesh(const std::string& filePath) {
    FaceMesh   mesh{.exists = std::filesystem::exists(filePath)};
    const auto record = [&mesh](const int status, const std::string& where) {
      mesh.calls.push_back(NetCDFCall{.status = status, .description = where});
    };

    int fileId = -1;
    record(nc_open(filePath.c_str(), NC_NOWRITE, &fileId), "nc_open(" + filePath + ")");

    int nodeDimensionId = -1;
    int faceDimensionId = -1;
    record(nc_inq_dimid(fileId, "nMesh2d_node", &nodeDimensionId), "inq nMesh2d_node");
    record(nc_inq_dimid(fileId, "nMesh2d_face", &faceDimensionId), "inq nMesh2d_face");
    record(nc_inq_dimlen(fileId, nodeDimensionId, &mesh.nodeCount), "len nMesh2d_node");
    record(nc_inq_dimlen(fileId, faceDimensionId, &mesh.faceCount), "len nMesh2d_face");
    mesh.nodeIndices = readIntegerVariable(fileId, "nMesh2d_node", mesh.nodeCount, mesh.calls);
    mesh.faceIndices = readIntegerVariable(fileId, "nMesh2d_face", mesh.faceCount, mesh.calls);

    int faceNodesVariableId = -1;
    record(nc_inq_varid(fileId, "face_nodes", &faceNodesVariableId), "inq face_nodes");
    const std::size_t start[2] = {0, 0};
    const std::size_t count[2] = {2, 4};
    record(nc_get_vara_int(fileId, faceNodesVariableId, start, count, mesh.faceNodes.data()), "read face_nodes");

    int materialVariableId = -1;
    record(nc_inq_varid(fileId, "Material", &materialVariableId), "inq Material");
    const std::size_t fieldStart[1] = {0};
    const std::size_t fieldCount[1] = {2};
    record(
      nc_get_vara_float(fileId, materialVariableId, fieldStart, fieldCount, mesh.material.data()),
      "read Material"
    );

    record(nc_close(fileId), "nc_close(" + filePath + ")");
    return mesh;
  }

  /**
   * Reads the node and face counts and records the status of every call.
   */
  FaceMesh readNodeAndFaceCounts(const std::string& filePath) {
    FaceMesh   mesh{.exists = std::filesystem::exists(filePath)};
    const auto record = [&mesh](const int status, const std::string& where) {
      mesh.calls.push_back(NetCDFCall{.status = status, .description = where});
    };

    int fileId = -1;
    record(nc_open(filePath.c_str(), NC_NOWRITE, &fileId), "nc_open(" + filePath + ")");
    int nodeDimensionId = -1;
    int faceDimensionId = -1;
    record(nc_inq_dimid(fileId, "nMesh2d_node", &nodeDimensionId), "inq nMesh2d_node");
    record(nc_inq_dimid(fileId, "nMesh2d_face", &faceDimensionId), "inq nMesh2d_face");
    record(nc_inq_dimlen(fileId, nodeDimensionId, &mesh.nodeCount), "len nMesh2d_node");
    record(nc_inq_dimlen(fileId, faceDimensionId, &mesh.faceCount), "len nMesh2d_face");
    record(nc_close(fileId), "nc_close(" + filePath + ")");
    return mesh;
  }

  void validateCalls(const std::vector<NetCDFCall>& calls) {
    for (const NetCDFCall& call : calls) {
      VALIDATE_EQUALS(call.status, NC_NOERR, call.description);
    }
  }

  void validateHexahedronMesh(const HexahedronMesh& mesh) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    VALIDATE(mesh.exists);
    validateCalls(mesh.calls);
    VALIDATE_EQUALS(mesh.topologyDimension, 3);
    VALIDATE(
      mesh.nodeCount == static_cast<std::size_t>(8 * ranks) and mesh.volumeCount == static_cast<std::size_t>(ranks),
      mesh.nodeCount,
      mesh.volumeCount,
      ranks
    );
    VALIDATE(isIndexSequence(mesh.nodeIndices) and isIndexSequence(mesh.volumeIndices));
    for (int node = 0; node < 8; node++) {
      VALIDATE_EQUALS(mesh.connectivity[node], node, node);
    }
    VALIDATE(mesh.nodeOrdering.find("VTK_HEXAHEDRON") != std::string::npos, mesh.nodeOrdering);
    VALIDATE_EQUALS(mesh.shapeTag, 2);
    const float expectedZ[8] = {0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    for (int node = 0; node < 8; node++) {
      VALIDATE_EQUALS(mesh.zCoordinates[node], expectedZ[node], node);
    }
    VALIDATE_EQUALS(mesh.location, std::string("volume"));
    VALIDATE_EQUALS(mesh.density, 2.5F);
  }

  void validateTwoQuadranglesPerRank(const FaceMesh& mesh, const int ranks) {
    VALIDATE(mesh.exists);
    validateCalls(mesh.calls);
    VALIDATE(
      mesh.nodeCount == static_cast<std::size_t>(8 * ranks) and mesh.faceCount == static_cast<std::size_t>(2 * ranks),
      mesh.nodeCount,
      mesh.faceCount,
      ranks
    );
    VALIDATE(isIndexSequence(mesh.nodeIndices) and isIndexSequence(mesh.faceIndices));
    for (int node = 0; node < 8; node++) {
      VALIDATE_EQUALS(mesh.faceNodes[node], node, node);
    }
    VALIDATE(mesh.material[0] == 3.0F and mesh.material[1] == 7.0F, mesh.material[0], mesh.material[1]);
  }

  /**
   * Writes mesh as the snapshot at the time 0 and returns the path of its file.
   */
  std::string writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::NetCDFFileWriter writer;
    writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
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

UNIT_TEST("The collective write of a unit square with point and cell data creates the file") {
  GIVEN("A unit square with the point field Pressure and the two-component cell field Velocity on every rank") {
    const std::string    filename = "NetCDFTestQuadrangle";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);
    mesh.pointFields.push_back({"Pressure", 1, {10.0, 11.0, 12.0, 13.0}});
    mesh.cellFields.push_back({"Velocity", 2, {1.5, -1.5}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master looks for the file") {
      const std::string path   = writeMesh(mesh, filename);
      const bool        exists = std::filesystem::exists(path);
      removeSeries(filename);

      THEN("The file exists on the global master") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          VALIDATE(exists, path);
        }
      }
    }
  }
}

UNIT_TEST("One unit cube per rank writes a 3D mesh with the node order, the coordinates and the density of the cubes") {
  GIVEN("A unit cube with the cell field Density 2.5 on every rank") {
    const std::string    filename = "NetCDFTestHexahedron";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitCube(mesh, 0.0);
    mesh.cellFields.push_back({"Density", 1, {2.5}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the file back") {
      const std::string    path = writeMesh(mesh, filename);
      const HexahedronMesh hexahedronMesh =
        tarch::mpi::Rank::getInstance().isGlobalMaster() ? readHexahedronMesh(path) : HexahedronMesh{};
      removeSeries(filename);

      THEN("The file contains eight indexed nodes and one indexed volume per rank, the node order, z and Density") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateHexahedronMesh(hexahedronMesh);
        }
      }
    }
  }
}

UNIT_TEST("Two unit squares per rank write one 2D mesh with the nodes, the faces and the cell data of every rank") {
  GIVEN("Two unit squares with the point field Pressure and the cell field Material 3 and 7 on every rank") {
    const std::string    filename = "NetCDFTestTwoQuadrangles";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 2);
    mesh.pointFields.push_back({"Pressure", 1, {10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0}});
    mesh.cellFields.push_back({"Material", 1, {3.0, 7.0}});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the file back") {
      const std::string path     = writeMesh(mesh, filename);
      const FaceMesh    faceMesh = tarch::mpi::Rank::getInstance().isGlobalMaster() ? readFaceMesh(path) : FaceMesh{};
      removeSeries(filename);

      THEN("The file contains eight indexed nodes and two indexed faces per rank with the nodes and Material") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          validateTwoQuadranglesPerRank(faceMesh, tarch::mpi::Rank::getInstance().getNumberOfRanks());
        }
      }
    }
  }
}

UNIT_TEST("The collective write completes when only the global master writes cells") {
  GIVEN("A unit square with the cell field Material on the global master and the empty field Material elsewhere") {
    const std::string    filename = "NetCDFTestHeterogeneous";
    tarch::plotter::Mesh mesh;
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);
      mesh.cellFields.push_back({"Material", 1, {3.0}});
    } else {
      mesh.cellFields.push_back({"Material", 1, {}});
    }

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master looks for the file") {
      const std::string path   = writeMesh(mesh, filename);
      const bool        exists = std::filesystem::exists(path);
      removeSeries(filename);

      THEN("The file exists on the global master") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          VALIDATE(exists, path);
        }
      }
    }
  }
}

UNIT_TEST("The collective write completes when the ranks write different non-zero cell counts") {
  GIVEN("Three unit squares with the cell field Material on the global master and two on every other rank") {
    const std::string    filename      = "NetCDFTestUneven";
    const int            numberOfCells = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(
      mesh,
      100.0 * tarch::mpi::Rank::getInstance().getRank(),
      numberOfCells
    );
    mesh.cellFields.push_back({"Material", 1, tarch::plotter::MeshArray<double>(numberOfCells, 1.0)});

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master reads the counts back") {
      const std::string path = writeMesh(mesh, filename);
      const FaceMesh    counts =
        tarch::mpi::Rank::getInstance().isGlobalMaster() ? readNodeAndFaceCounts(path) : FaceMesh{};
      removeSeries(filename);
      const int         otherRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks() - 1;
      const std::size_t faces      = 3 + 2 * static_cast<std::size_t>(otherRanks);

      THEN("The file contains the faces of every rank with four nodes each") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          VALIDATE(counts.exists);
          validateCalls(counts.calls);
          VALIDATE_EQUALS(counts.faceCount, faces);
          VALIDATE_EQUALS(counts.nodeCount, 4 * faces);
        }
      }
    }
  }
}

UNIT_TEST("A mesh without points, cells and fields writes a valid file") {
  GIVEN("An empty mesh on every rank") {
    const std::string          filename = "NetCDFTestEmpty";
    const tarch::plotter::Mesh mesh;

    WHEN("The writer writes the mesh as the snapshot at the time 0 and the global master looks for the file") {
      const std::string path   = writeMesh(mesh, filename);
      const bool        exists = std::filesystem::exists(path);
      removeSeries(filename);

      THEN("The file exists on the global master") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          VALIDATE(exists, path);
        }
      }
    }
  }
}

UNIT_TEST("The .nc.series file lists every snapshot file that one writer writes once") {
  GIVEN("A mesh of one unit square on every rank") {
    const std::string    filename = "NetCDFTestSeries";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);

    WHEN("One writer writes the mesh at the times 0, 0.5 and 0.5 and the global master reads the series back") {
      tarch::plotter::NetCDFFileWriter writer;
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.5);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.5);
      barrier();
      std::string content;
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        const std::ifstream stream(getPath(filename + ".nc.series"));
        std::stringstream   buffer;
        buffer << stream.rdbuf();
        content = buffer.str();
        std::filesystem::remove(getPath(filename + "-0_000000.nc"));
        std::filesystem::remove(getPath(filename + "-0_500000.nc"));
        std::filesystem::remove(getPath(filename + ".nc.series"));
      }
      barrier();

      THEN("The series of the global master lists both snapshots once with their times") {
        if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
          const std::string lastEntry = R"({ "name" : "NetCDFTestSeries-0_500000.nc", "time" : 0.5 })";
          VALIDATE(content.find(R"({ "name" : "NetCDFTestSeries-0_000000.nc", "time" : 0 },)") != std::string::npos);
          VALIDATE(content.find(lastEntry) != std::string::npos);
          VALIDATE(content.find(lastEntry) == content.rfind(lastEntry));
        }
      }
    }
  }
}

#endif
