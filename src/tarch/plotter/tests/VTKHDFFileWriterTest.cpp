// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/VTKHDFFileWriter.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  int getNumberOfRanks() { return tarch::mpi::Rank::getInstance().getNumberOfRanks(); }
  int getRank() { return tarch::mpi::Rank::getInstance().getRank(); }
  bool isMaster() { return tarch::mpi::Rank::getInstance().isGlobalMaster(); }

  void check(const bool condition, const std::string& message) {
    if (not condition) {
      throw std::runtime_error("VTKHDFFileWriterTest failed: " + message);
    }
  }

  // The global master calls the read-back helpers once the collective write has closed the
  // file, and each of them reads a whole dataset.
  std::vector<long long> readInt64Dataset(const hid_t file, const std::string& path) {
    const hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    const hid_t            dataSpace       = H5Dget_space(dataset);
    hsize_t                dimensions[2]   = {0, 0};
    const int              rank            = H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    const hsize_t          numberOfEntries = (rank == 2) ? dimensions[0] * dimensions[1] : dimensions[0];
    std::vector<long long> values(numberOfEntries);
    if (numberOfEntries > 0) {
      H5Dread(dataset, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return values;
  }

  std::vector<int> readUnsignedByteDataset(const hid_t file, const std::string& path) {
    const hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    const hid_t dataSpace     = H5Dget_space(dataset);
    hsize_t     dimensions[1] = {0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    std::vector<unsigned char> raw(dimensions[0]);
    if (dimensions[0] > 0) {
      H5Dread(dataset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, raw.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return std::vector<int>(raw.begin(), raw.end());
  }

  std::vector<float> readFloat32Dataset(const hid_t file, const std::string& path) {
    const hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    const hid_t        dataSpace       = H5Dget_space(dataset);
    hsize_t            dimensions[2]   = {0, 0};
    const int          rank            = H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    const hsize_t      numberOfEntries = (rank == 2) ? dimensions[0] * dimensions[1] : dimensions[0];
    std::vector<float> values(numberOfEntries);
    if (numberOfEntries > 0) {
      H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return values;
  }

  std::vector<long long> readVersion(const hid_t file) {
    const hid_t            group     = H5Gopen(file, "/VTKHDF", H5P_DEFAULT);
    const hid_t            attribute = H5Aopen(group, "Version", H5P_DEFAULT);
    std::vector<long long> version(2, 0);
    H5Aread(attribute, H5T_NATIVE_LLONG, version.data());
    H5Aclose(attribute);
    H5Gclose(group);
    return version;
  }

  long long readNumberOfSteps(const hid_t file) {
    const hid_t group         = H5Gopen(file, "/VTKHDF/Steps", H5P_DEFAULT);
    const hid_t attribute     = H5Aopen(group, "NSteps", H5P_DEFAULT);
    long long   numberOfSteps = 0;
    H5Aread(attribute, H5T_NATIVE_LLONG, &numberOfSteps);
    H5Aclose(attribute);
    H5Gclose(group);
    return numberOfSteps;
  }

  hid_t openForRead(const std::string& path) {
    const hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    check(file >= 0, "Could not reopen " + path);
    return file;
  }

  void removeFile(const std::string& path) {
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
  }
} // namespace

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void
tarch::plotter::tests::VTKHDFFileWriterTest::run() {
  // The read-back helpers report a malformed file by throwing, and the test framework does not
  // guard run(), so this catch records the failure and keeps the binary alive.
  try {
    testMethod(testSingleQuad2D);
    testMethod(testVertexMerging);
    testMethod(testTwoTimeSteps);
    testMethod(testMultipleSubdomains);
    testMethod(testParticlePointCloud);
    testMethod(test3DHexahedron);
    testMethod(testHeterogeneousRanks);
    testMethod(testUnevenSubdomainCounts);
    testMethod(testEmptyPlotter);
    testMethod(testInconsistentDataSizeThrows);
    testMethod(testInvalidIdentifierThrows);
    testMethod(testPrecisionSwitch);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

/**
 * A single step of one quad per rank writes consistent sizes, offsets, coordinates and cell data.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testSingleQuad2D() {
  const std::string filename = "VTKHDFTestSingleQuad";
  const std::string path     = filename + ".vtkhdf";

  // Every rank contributes one quad and shifts it in x by twice its rank number, so the
  // points are distinct. The writer merges them into one part and rebases the connectivity of
  // rank r onto the points 4r to 4r+3.
  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto         vertexPlotter   = plotter.createVertexPlotter(0);
    auto         cellPlotter     = plotter.createCellPlotter(0);
    auto         cellDataPlotter = plotter.createCellDataPlotter(0, "v", 1);
    const double offset          = 2.0 * getRank();
    const int    firstIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
    const int    secondIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
    const int    thirdIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 1.0});
    const int    fourthIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
    const int    quadrangle[4]   = {firstIndex, secondIndex, thirdIndex, fourthIndex};
    const int    cellIndex       = cellPlotter->plotQuadrangle(quadrangle);
    cellDataPlotter->plotCellData(cellIndex, static_cast<double>(getRank()));
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    check(readNumberOfSteps(file) == 1, "NSteps should be 1");
    // The writer declares the lowest specification version its features need, because a reader
    // rejects a file with a newer version than it supports.
    const auto version = readVersion(file);
    check(version[0] == 2 and version[1] == 0, "VTKHDF version should be 2.0");
    const auto        numberOfPoints          = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto        numberOfCells           = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto        numberOfConnectivityIds = readInt64Dataset(file, "/VTKHDF/NumberOfConnectivityIds");
    const std::string numberOfPointsInfo      = "size=" + std::to_string(numberOfPoints.size())
      + " value=" + (numberOfPoints.empty() ? std::string("none") : std::to_string(numberOfPoints[0]));
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 4 * numberOfRanks,
      "NumberOfPoints must be one per-step total of 4*ranks (ranks=" + std::to_string(numberOfRanks) + ", got "
        + numberOfPointsInfo + ")"
    );
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == numberOfRanks,
      "NumberOfCells must be one per-step total of ranks (single part)"
    );
    check(
      numberOfConnectivityIds.size() == 1 and numberOfConnectivityIds[0] == 4 * numberOfRanks,
      "NumberOfConnectivityIds total must be 4*ranks"
    );

    const auto types = readUnsignedByteDataset(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == numberOfRanks, "Types holds one entry per cell");
    for (const int type : types) {
      check(type == 9, "Every cell must be a VTK quad (9)");
    }

    const auto offsets = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == numberOfRanks + 1, "Offsets must have cells+1 entries (single part)");
    for (int cellIndex = 0; cellIndex <= numberOfRanks; cellIndex++) {
      check(offsets[cellIndex] == 4 * cellIndex, "Offsets must be the global cumulative 0,4,8,...");
    }

    const auto connectivity = readInt64Dataset(file, "/VTKHDF/Connectivity");
    check(static_cast<int>(connectivity.size()) == 4 * numberOfRanks, "Connectivity total");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      for (int nodeIndex = 0; nodeIndex < 4; nodeIndex++) {
        check(
          connectivity[4 * rankIndex + nodeIndex] == 4 * rankIndex + nodeIndex,
          "Connectivity must be rebased to global point indices per rank"
        );
      }
    }

    const auto cellData = readFloat32Dataset(file, "/VTKHDF/CellData/v");
    check(static_cast<int>(cellData.size()) == numberOfRanks, "One cell value per cell");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(
        cellData[rankIndex] == static_cast<float>(rankIndex),
        "Cell value must equal the contributing rank id, in rank order"
      );
    }

    const auto points = readFloat32Dataset(file, "/VTKHDF/Points");
    check(static_cast<int>(points.size()) == 4 * numberOfRanks * 3, "Points are (4*ranks, 3)");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(points[(4 * rankIndex) * 3 + 0] == static_cast<float>(2 * rankIndex), "Rank quad x-origin");
      check(points[(4 * rankIndex) * 3 + 2] == 0.0F, "2D points get z==0");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Two coincident vertices map onto one point under VertexMerging::Merged and onto two points
 * under VertexMerging::Distinct.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testVertexMerging() {
  const std::string filename = "VTKHDFTestVertexMerging";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    plotter.acquirePiece(1);
    {
      auto mergingVertexPlotter = plotter.createVertexPlotter(0, tarch::plotter::FileWriter::VertexMerging::Merged);
      auto mergingDataPlotter   = plotter.createVertexDataPlotter(0, "Value", 1);
      const auto position       = tarch::la::Vector<2, double>{4.0 * getRank(), 0.0};
      const auto firstVertex    = mergingVertexPlotter->plotVertex(position);
      const auto secondVertex   = mergingVertexPlotter->plotVertex(position);
      check(firstVertex == secondVertex, "Coincident vertices merge onto one index");
      mergingDataPlotter->plotVertexData(firstVertex, 1.0);
    }
    {
      auto distinctVertexPlotter = plotter.createVertexPlotter(1, tarch::plotter::FileWriter::VertexMerging::Distinct);
      auto distinctDataPlotter   = plotter.createVertexDataPlotter(1, "Value", 1);
      const auto position        = tarch::la::Vector<2, double>{4.0 * getRank() + 2.0, 0.0};
      const auto firstVertex     = distinctVertexPlotter->plotVertex(position);
      const auto secondVertex    = distinctVertexPlotter->plotVertex(position);
      check(firstVertex != secondVertex, "Distinct coincident vertices use separate indices");
      distinctDataPlotter->plotVertexData(firstVertex, 2.0);
      distinctDataPlotter->plotVertexData(secondVertex, 3.0);
    }
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const auto numberOfRanks  = getNumberOfRanks();
    const auto file           = openForRead(path);
    const auto numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto points         = readFloat32Dataset(file, "/VTKHDF/Points");
    const auto values         = readFloat32Dataset(file, "/VTKHDF/PointData/Value");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 3 * numberOfRanks,
      "Each rank writes one merged point and two distinct points"
    );
    for (int rank = 0; rank < numberOfRanks; rank++) {
      const auto firstDistinctPoint  = 3 * rank + 1;
      const auto secondDistinctPoint = 3 * rank + 2;
      for (int coordinate = 0; coordinate < 3; coordinate++) {
        check(
          points[3 * firstDistinctPoint + coordinate] == points[3 * secondDistinctPoint + coordinate],
          "Distinct vertices retain coincident coordinates"
        );
      }
      check(values[3 * rank] == 1.0F, "The merged point retains its value");
      check(values[firstDistinctPoint] == 2.0F, "The first distinct point retains its value");
      check(values[secondDistinctPoint] == 3.0F, "The second distinct point retains its value");
    }
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Two writes into one file raise NSteps to 2 and fill the Steps offsets with the cumulative
 * sizes of the preceding steps.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testTwoTimeSteps() {
  const std::string filename = "VTKHDFTestTwoSteps";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto         vertexPlotter = plotter.createVertexPlotter(0);
    auto         cellPlotter   = plotter.createCellPlotter(0);
    const double offset        = 2.0 * getRank();
    const int    firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
    const int    secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
    const int    thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 1.0});
    const int    fourthIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
    const int    quadrangle[4] = {firstIndex, secondIndex, thirdIndex, fourthIndex};
    cellPlotter->plotQuadrangle(quadrangle);
    plotter.writeToFile(filename, 0.0);
    plotter.writeToFile(filename, 0.5);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    check(readNumberOfSteps(file) == 2, "Two appends must give NSteps==2");
    const auto values = readFloat32Dataset(file, "/VTKHDF/Steps/Values");
    check(values.size() == 2 and values[0] == 0.0F and values[1] == 0.5F, "Steps/Values must be the two times");

    const auto pointOffsets        = readInt64Dataset(file, "/VTKHDF/Steps/PointOffsets");
    const auto cellOffsets         = readInt64Dataset(file, "/VTKHDF/Steps/CellOffsets");
    const auto connectivityOffsets = readInt64Dataset(file, "/VTKHDF/Steps/ConnectivityIdOffsets");
    const auto partOffsets         = readInt64Dataset(file, "/VTKHDF/Steps/PartOffsets");
    check(
      pointOffsets.size() == 2 and pointOffsets[0] == 0 and pointOffsets[1] == 4 * numberOfRanks,
      "PointOffsets are cumulative prior-step points"
    );
    check(
      cellOffsets.size() == 2 and cellOffsets[0] == 0 and cellOffsets[1] == numberOfRanks,
      "CellOffsets are cumulative prior-step cells"
    );
    check(
      connectivityOffsets.size() == 2 and connectivityOffsets[0] == 0 and connectivityOffsets[1] == 4 * numberOfRanks,
      "ConnectivityIdOffsets are cumulative prior-step connectivity entries"
    );
    check(
      partOffsets.size() == 2 and partOffsets[0] == 0 and partOffsets[1] == 1,
      "PartOffsets are cumulative parts (1 part/step)"
    );

    // The geometry arrays hold both steps one after the other.
    const auto numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    check(
      numberOfPoints.size() == 2 and numberOfPoints[0] == 4 * numberOfRanks and numberOfPoints[1] == 4 * numberOfRanks,
      "Two per-step point totals"
    );

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Two subdomains merge into a single part of the step, and the connectivity of the second
 * subdomain is rebased past the points of the first.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testMultipleSubdomains() {
  const std::string filename = "VTKHDFTestMultiSubdomain";
  const std::string path     = filename + ".vtkhdf";

  // Every rank holds a triangle of three points in one subdomain and a quad of four points in
  // the other, and the two merge into one part of two cells and seven points.
  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    plotter.acquirePiece(1);
    const double offset = 10.0 * getRank();
    {
      auto      vertexPlotter = plotter.createVertexPlotter(0);
      auto      cellPlotter   = plotter.createCellPlotter(0);
      const int firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
      const int secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
      const int thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
      const int triangle[3]   = {firstIndex, secondIndex, thirdIndex};
      cellPlotter->plotTriangle(triangle);
    }
    {
      auto      vertexPlotter = plotter.createVertexPlotter(1);
      auto      cellPlotter   = plotter.createCellPlotter(1);
      const int firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 2.0, 0.0});
      const int secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 3.0, 0.0});
      const int thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 3.0, 1.0});
      const int fourthIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 2.0, 1.0});
      const int quadrangle[4] = {firstIndex, secondIndex, thirdIndex, fourthIndex};
      cellPlotter->plotQuadrangle(quadrangle);
    }
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    const auto numberOfPoints          = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto numberOfCells           = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto numberOfConnectivityIds = readInt64Dataset(file, "/VTKHDF/NumberOfConnectivityIds");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 7 * numberOfRanks,
      "Two subdomains merge to 7*ranks points (one entry)"
    );
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == 2 * numberOfRanks,
      "Merged cell count is 2*ranks (single part)"
    );
    check(
      numberOfConnectivityIds.size() == 1 and numberOfConnectivityIds[0] == 7 * numberOfRanks,
      "Merged connectivity is 7*ranks"
    );

    const auto types = readUnsignedByteDataset(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == 2 * numberOfRanks, "Two cells per rank");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(types[2 * rankIndex] == 5, "First cell of each rank is a triangle (5)");
      check(types[2 * rankIndex + 1] == 9, "Second cell of each rank is a quad (9)");
    }

    const auto offsets = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == 2 * numberOfRanks + 1, "Offsets holds cells+1 entries (single part)");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(offsets[2 * rankIndex] == 7 * rankIndex, "Before-offset of the rank's triangle");
      check(
        offsets[2 * rankIndex + 1] == 7 * rankIndex + 3,
        "Before-offset of the rank's quad, past the triangle's 3 nodes"
      );
    }
    check(offsets[2 * numberOfRanks] == 7 * numberOfRanks, "Trailing sentinel equals the total connectivity");

    const auto connectivity = readInt64Dataset(file, "/VTKHDF/Connectivity");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      // The triangle owns the nodes 7r to 7r+2 and the quad the nodes 7r+3 to 7r+6.
      check(connectivity[7 * rankIndex + 0] == 7 * rankIndex + 0, "Triangle node rebase");
      check(connectivity[7 * rankIndex + 3] == 7 * rankIndex + 3, "Quad node rebase past the triangle");
      check(connectivity[7 * rankIndex + 6] == 7 * rankIndex + 6, "Quad last node rebase");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A grid of vertices and vertex data without cells writes NumberOfCells 0 and Offsets [0].
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testParticlePointCloud() {
  const std::string filename = "VTKHDFTestParticles";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto      vertexPlotter     = plotter.createVertexPlotter(0);
    auto      vertexDataPlotter = plotter.createVertexDataPlotter(0, "Mass", 1);
    const int firstVertex       = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.5 + getRank(), 0.5});
    vertexDataPlotter->plotVertexData(firstVertex, 10.0);
    const int secondVertex = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.6 + getRank(), 0.6});
    vertexDataPlotter->plotVertexData(secondVertex, 20.0);
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    const auto numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto offsets        = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(numberOfPoints.size() == 1 and numberOfPoints[0] == 2 * numberOfRanks, "Point cloud has 2*ranks points");
    check(numberOfCells.size() == 1 and numberOfCells[0] == 0, "Point cloud has no cells");
    check(offsets.size() == 1 and offsets[0] == 0, "Offsets is the [0] sentinel alone for a cell-less grid");

    const auto mass = readFloat32Dataset(file, "/VTKHDF/PointData/Mass");
    check(static_cast<int>(mass.size()) == 2 * numberOfRanks, "One Mass value per point");

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A hexahedron with cell data and non-zero z coordinates writes its 3D geometry and connectivity.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::test3DHexahedron() {
  const std::string filename = "VTKHDFTest3DHex";
  const std::string path     = filename + ".vtkhdf";

  // Every rank contributes one unit cube of eight corners and shifts it in x by twice its
  // rank number.
  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto         vertexPlotter    = plotter.createVertexPlotter(0);
    auto         cellPlotter      = plotter.createCellPlotter(0);
    auto         cellDataPlotter  = plotter.createCellDataPlotter(0, "rho", 1);
    const double offset           = 2.0 * getRank();
    const double cornerX[8]       = {0, 1, 1, 0, 0, 1, 1, 0};
    const double cornerY[8]       = {0, 0, 1, 1, 0, 0, 1, 1};
    const double cornerZ[8]       = {0, 0, 0, 0, 1, 1, 1, 1};
    int          cornerIndices[8] = {};
    for (int cornerIndex = 0; cornerIndex < 8; cornerIndex++) {
      cornerIndices[cornerIndex] = vertexPlotter->plotVertex(
        tarch::la::Vector<3, double>{offset + cornerX[cornerIndex], cornerY[cornerIndex], cornerZ[cornerIndex]}
      );
    }
    const int cellIndex = cellPlotter->plotHexahedron(cornerIndices);
    cellDataPlotter->plotCellData(cellIndex, 2.5);
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks  = getNumberOfRanks();
    const hid_t file           = openForRead(path);
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  types          = readUnsignedByteDataset(file, "/VTKHDF/Types");
    const auto  density        = readFloat32Dataset(file, "/VTKHDF/CellData/rho");
    const auto  points         = readFloat32Dataset(file, "/VTKHDF/Points");
    check(numberOfCells.size() == 1 and numberOfCells[0] == numberOfRanks, "One hexahedron per rank, merged");
    check(numberOfPoints.size() == 1 and numberOfPoints[0] == 8 * numberOfRanks, "Eight nodes per hexahedron");
    check(static_cast<int>(types.size()) == numberOfRanks, "One cell type per cell");
    for (const int type : types) {
      check(type == 12, "VTK hexahedron type is 12");
    }
    check(static_cast<int>(density.size()) == numberOfRanks, "One cell value per hexahedron");
    for (const float value : density) {
      check(value == 2.5F, "Hexahedron cell value round-trips");
    }
    check(static_cast<int>(points.size()) == 8 * numberOfRanks * 3, "3D points are (8*ranks, 3)");
    // The top face of the cube contributes the non-zero z coordinates.
    bool anyNonZeroZ = false;
    for (std::size_t pointIndex = 0; pointIndex < points.size() / 3; pointIndex++) {
      if (points[3 * pointIndex + 2] != 0.0F) {
        anyNonZeroZ = true;
      }
    }
    check(anyNonZeroZ, "3D geometry has non-zero z coordinates");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A write in which only the global master plots cells finishes and stores the master's cells.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testHeterogeneousRanks() {
  const std::string filename = "VTKHDFTestHeterogeneous";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    if (isMaster()) {
      auto      vertexPlotter   = plotter.createVertexPlotter(0);
      auto      cellPlotter     = plotter.createCellPlotter(0);
      auto      cellDataPlotter = plotter.createCellDataPlotter(0, "Marker", 1);
      const int firstIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      const int secondIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      const int thirdIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      const int fourthIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      const int quadrangle[4]   = {firstIndex, secondIndex, thirdIndex, fourthIndex};
      const int cellIndex       = cellPlotter->plotQuadrangle(quadrangle);
      cellDataPlotter->plotCellData(cellIndex, 7.0);
    }
    // Every rank enters this collective call.
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const hid_t file           = openForRead(path);
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  marker         = readFloat32Dataset(file, "/VTKHDF/CellData/Marker");
    check(numberOfPoints.size() == 1 and numberOfPoints[0] == 4, "Only the master's 4 points");
    check(numberOfCells.size() == 1 and numberOfCells[0] == 1, "Only the master's 1 cell");
    check(marker.size() == 1 and marker[0] == 7.0F, "The master's cell value survived");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Ranks with different subdomain counts enter the same collective calls.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "VTKHDFTestUneven";
  const std::string path     = filename + ".vtkhdf";

  const int numberOfSubdomains = isMaster() ? 3 : 2;
  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    for (int subdomainIndex = 0; subdomainIndex < numberOfSubdomains; subdomainIndex++) {
      plotter.acquirePiece(subdomainIndex);
    }
    for (int subdomainIndex = 0; subdomainIndex < numberOfSubdomains; subdomainIndex++) {
      auto         vertexPlotter = plotter.createVertexPlotter(subdomainIndex);
      auto         cellPlotter   = plotter.createCellPlotter(subdomainIndex);
      const double offset        = 100.0 * getRank() + 2.0 * subdomainIndex;
      const int    firstIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
      const int    secondIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
      const int    thirdIndex    = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 1.0});
      const int    fourthIndex   = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
      const int    quadrangle[4] = {firstIndex, secondIndex, thirdIndex, fourthIndex};
      cellPlotter->plotQuadrangle(quadrangle);
    }
    // Every rank enters this collective call once, whatever its subdomain count.
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const long long expectedNumberOfCells = 3 + 2 * (getNumberOfRanks() - 1);
    const hid_t     file                  = openForRead(path);
    const auto      numberOfCells         = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto      numberOfPoints        = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == expectedNumberOfCells,
      "Uneven distribution merges to one part with the summed cell total"
    );
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 4 * expectedNumberOfCells,
      "Uneven distribution merged node total"
    );
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void
tarch::plotter::tests::VTKHDFFileWriterTest::testEmptyPlotter() {
  const std::string filename = "VTKHDFTestEmpty";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    // Every rank enters this collective call with an empty grid.
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const hid_t file           = openForRead(path);
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  offsets        = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(readNumberOfSteps(file) == 1, "An empty write still records a step");
    check(numberOfPoints.size() == 1 and numberOfPoints[0] == 0, "Empty grid has 0 points");
    check(numberOfCells.size() == 1 and numberOfCells[0] == 0, "Empty grid has 0 cells");
    check(offsets.size() == 1 and offsets[0] == 0, "Empty grid Offsets is [0]");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void
tarch::plotter::tests::VTKHDFFileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "VTKHDFTestInconsistent";

  bool exceptionThrown = false;
  try {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto vertexPlotter     = plotter.createVertexPlotter(0);
    auto vertexDataPlotter = plotter.createVertexDataPlotter(0, "Broken", 1);
    vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    // The write validation rejects a field with a value for one of the two vertices only.
    vertexDataPlotter->plotVertexData(0, 5.0);
    plotter.writeToFile(filename, 0.0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }
  if (isMaster()) {
    removeFile(filename + ".vtkhdf");
  }
  check(exceptionThrown, "Inconsistent field size must be rejected on write");
}

void
tarch::plotter::tests::VTKHDFFileWriterTest::testInvalidIdentifierThrows() {
  tarch::plotter::VTKHDFFileWriter plotter(".");
  plotter.acquirePiece(0);
  bool exceptionThrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }
  check(exceptionThrown, "An illegal data identifier must be rejected");
}

/**
 * Single-precision output stores 4-byte reals, and double-precision output stores 8-byte reals
 * without rounding.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testPrecisionSwitch() {
  // The value differs from 1.0 by less than the single-precision resolution, so only
  // double-precision output keeps it exactly.
  const double fineValue = 1.0 + 1.0e-12;

  auto writeQuadWithValue = [&](const std::string& filename) {
    tarch::plotter::VTKHDFFileWriter plotter(".");
    plotter.acquirePiece(0);
    auto         vertexPlotter   = plotter.createVertexPlotter(0);
    auto         cellPlotter     = plotter.createCellPlotter(0);
    auto         cellDataPlotter = plotter.createCellDataPlotter(0, "v", 1);
    const double offset          = 2.0 * getRank();
    const int    firstIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 0.0});
    const int    secondIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 0.0});
    const int    thirdIndex      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 1.0, 1.0});
    const int    fourthIndex     = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{offset + 0.0, 1.0});
    const int    quadrangle[4]   = {firstIndex, secondIndex, thirdIndex, fourthIndex};
    const int    cellIndex       = cellPlotter->plotQuadrangle(quadrangle);
    cellDataPlotter->plotCellData(cellIndex, fineValue);
    plotter.writeToFile(filename, 0.0);
  };

  auto getStorageBytes = [](const hid_t file, const std::string& path) {
    const hid_t       dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    const hid_t       type    = H5Dget_type(dataset);
    const std::size_t bytes   = H5Tget_size(type);
    H5Tclose(type);
    H5Dclose(dataset);
    return bytes;
  };

  const std::string singleName = "VTKHDFTestSinglePrecision";
  const std::string doubleName = "VTKHDFTestDoublePrecision";

  check(not tarch::plotter::FileWriter::getDoublePrecisionOutput(), "Single precision is the default");
  writeQuadWithValue(singleName);

  tarch::plotter::FileWriter::setDoublePrecisionOutput(true);
  writeQuadWithValue(doubleName);
  tarch::plotter::FileWriter::setDoublePrecisionOutput(false);

  barrier();
  if (isMaster()) {
    const hid_t singleFile = openForRead(singleName + ".vtkhdf");
    check(getStorageBytes(singleFile, "/VTKHDF/Points") == 4, "Single-precision output stores 4-byte points");
    check(getStorageBytes(singleFile, "/VTKHDF/CellData/v") == 4, "Single-precision output stores 4-byte fields");
    H5Fclose(singleFile);
    removeFile(singleName + ".vtkhdf");

    const hid_t doubleFile = openForRead(doubleName + ".vtkhdf");
    check(getStorageBytes(doubleFile, "/VTKHDF/Points") == 8, "Double-precision output stores 8-byte points");
    check(getStorageBytes(doubleFile, "/VTKHDF/CellData/v") == 8, "Double-precision output stores 8-byte fields");

    const hid_t dataset       = H5Dopen(doubleFile, "/VTKHDF/CellData/v", H5P_DEFAULT);
    const hid_t dataSpace     = H5Dget_space(dataset);
    hsize_t     dimensions[2] = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    std::vector<double> values(dimensions[0]);
    H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    for (const double value : values) {
      check(value == fineValue, "Double-precision output round-trips below single-precision resolution");
    }

    H5Fclose(doubleFile);
    removeFile(doubleName + ".vtkhdf");
  }
  barrier();
}

#else // No HDF5

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void
tarch::plotter::tests::VTKHDFFileWriterTest::run() {}

#endif
