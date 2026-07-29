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

  int  ranks() { return tarch::mpi::Rank::getInstance().getNumberOfRanks(); }
  int  thisRank() { return tarch::mpi::Rank::getInstance().getRank(); }
  bool isMaster() { return tarch::mpi::Rank::getInstance().isGlobalMaster(); }

  void check(bool condition, const std::string& message) {
    if (not condition) {
      throw std::runtime_error("VTKHDFFileWriterTest failed: " + message);
    }
  }

  // Serial read-back helpers (master only, after the collective write closed the
  // file). Each reads a whole dataset; the file is small.
  std::vector<long long> readI64(hid_t file, const std::string& path) {
    hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    hid_t                  dataSpace       = H5Dget_space(dataset);
    hsize_t                dims[2]         = {0, 0};
    const int              rank            = H5Sget_simple_extent_dims(dataSpace, dims, nullptr);
    const hsize_t          numberOfEntries = (rank == 2) ? dims[0] * dims[1] : dims[0];
    std::vector<long long> out(numberOfEntries);
    if (numberOfEntries > 0) {
      H5Dread(dataset, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return out;
  }

  std::vector<int> readU8AsInt(hid_t file, const std::string& path) {
    hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    hid_t   dataSpace = H5Dget_space(dataset);
    hsize_t dims[1]   = {0};
    H5Sget_simple_extent_dims(dataSpace, dims, nullptr);
    std::vector<unsigned char> raw(dims[0]);
    if (dims[0] > 0) {
      H5Dread(dataset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, raw.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return std::vector<int>(raw.begin(), raw.end());
  }

  std::vector<float> readF32(hid_t file, const std::string& path) {
    hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "Missing dataset " + path);
    hid_t              dataSpace       = H5Dget_space(dataset);
    hsize_t            dims[2]         = {0, 0};
    const int          rank            = H5Sget_simple_extent_dims(dataSpace, dims, nullptr);
    const hsize_t      numberOfEntries = (rank == 2) ? dims[0] * dims[1] : dims[0];
    std::vector<float> out(numberOfEntries);
    if (numberOfEntries > 0) {
      H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
    }
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    return out;
  }

  std::vector<long long> readVersion(hid_t file) {
    hid_t                  group     = H5Gopen(file, "/VTKHDF", H5P_DEFAULT);
    hid_t                  attribute = H5Aopen(group, "Version", H5P_DEFAULT);
    std::vector<long long> version(2, 0);
    H5Aread(attribute, H5T_NATIVE_LLONG, version.data());
    H5Aclose(attribute);
    H5Gclose(group);
    return version;
  }

  long long readNSteps(hid_t file) {
    hid_t     group         = H5Gopen(file, "/VTKHDF/Steps", H5P_DEFAULT);
    hid_t     attribute     = H5Aopen(group, "NSteps", H5P_DEFAULT);
    long long numberOfSteps = 0;
    H5Aread(attribute, H5T_NATIVE_LLONG, &numberOfSteps);
    H5Aclose(attribute);
    H5Gclose(group);
    return numberOfSteps;
  }

  hid_t openForRead(const std::string& path) {
    hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
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
  TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void tarch::plotter::tests::VTKHDFFileWriterTest::run() {
  // check() and the read-back helpers report a malformed file by throwing. Without this
  // catch the exception leaves run(), which the test framework does not guard, and the whole
  // unit-test binary aborts instead of recording a failed case. The remaining cases are then
  // skipped, so the counter has to be raised here as well.
  try {
    testMethod(testSingleQuad2D);
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
 * One quad, one step: checks the NumberOf* per-step totals, Offsets=[0,4], the
 * vertex coordinates and the cell-data value round-trip.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testSingleQuad2D() {
  const std::string filename = "VTKHDFTestSingleQuad";
  const std::string path     = filename + ".vtkhdf";

  // Every rank contributes one quad, shifted in x by 2*rank so the points are
  // distinct. The writer must merge them into one part whose connectivity is
  // rebased: rank r's quad must reference points [4r, 4r+3].
  {
    VTKHDFFileWriter plotter(".", 1);
    auto             v       = plotter.createVertexPlotter(0);
    auto             c       = plotter.createCellPlotter(0);
    auto             cd      = plotter.createCellDataPlotter(0, "v", 1);
    const double     s       = 2.0 * thisRank();
    int              p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
    int              p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
    int              p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
    int              p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
    int              quad[4] = {p0, p1, p2, p3};
    int              cell    = c->plotQuadrangle(quad);
    cd->plotCellData(cell, static_cast<double>(thisRank()));
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int numberOfRanks = ranks();
    hid_t     file          = openForRead(path);

    check(readNSteps(file) == 1, "NSteps should be 1");
    // The declared specification version stays at the floor the file's features
    // need; a higher number makes readers that could parse the file reject it.
    const auto version = readVersion(file);
    check(version[0] == 2 and version[1] == 0, "VTKHDF version should be 2.0");
    const auto        nPoints     = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto        nCells      = readI64(file, "/VTKHDF/NumberOfCells");
    const auto        nConn       = readI64(file, "/VTKHDF/NumberOfConnectivityIds");
    const std::string nPointsInfo = "size=" + std::to_string(nPoints.size())
      + " value=" + (nPoints.empty() ? std::string("none") : std::to_string(nPoints[0]));
    check(
      nPoints.size() == 1 and nPoints[0] == 4 * numberOfRanks,
      "NumberOfPoints must be one per-step total of 4*ranks (ranks=" + std::to_string(numberOfRanks) + ", got "
        + nPointsInfo + ")"
    );
    check(
      nCells.size() == 1 and nCells[0] == numberOfRanks,
      "NumberOfCells must be one per-step total of ranks (single part)"
    );
    check(nConn.size() == 1 and nConn[0] == 4 * numberOfRanks, "NumberOfConnectivityIds total must be 4*ranks");

    const auto types = readU8AsInt(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == numberOfRanks, "Types holds one entry per cell");
    for (int t : types) {
      check(t == 9, "Every cell must be a VTK quad (9)");
    }

    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == numberOfRanks + 1, "Offsets must have cells+1 entries (single part)");
    for (int i = 0; i <= numberOfRanks; i++) {
      check(offsets[i] == 4 * i, "Offsets must be the global cumulative 0,4,8,...");
    }

    const auto conn = readI64(file, "/VTKHDF/Connectivity");
    check(static_cast<int>(conn.size()) == 4 * numberOfRanks, "Connectivity total");
    for (int r = 0; r < numberOfRanks; r++) {
      for (int k = 0; k < 4; k++) {
        check(conn[4 * r + k] == 4 * r + k, "Connectivity must be rebased to global point indices per rank");
      }
    }

    const auto cellData = readF32(file, "/VTKHDF/CellData/v");
    check(static_cast<int>(cellData.size()) == numberOfRanks, "One cell value per cell");
    for (int r = 0; r < numberOfRanks; r++) {
      check(cellData[r] == static_cast<float>(r), "Cell value must equal the contributing rank id, in rank order");
    }

    const auto points = readF32(file, "/VTKHDF/Points");
    check(static_cast<int>(points.size()) == 4 * numberOfRanks * 3, "Points are (4*ranks, 3)");
    for (int r = 0; r < numberOfRanks; r++) {
      check(points[(4 * r) * 3 + 0] == static_cast<float>(2 * r), "Rank quad x-origin");
      check(points[(4 * r) * 3 + 2] == 0.0f, "2D points get z==0");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Two traversals append to one file: NSteps grows to 2 and the Steps offsets are
 * the cumulative prior-step sizes, which a reader uses to slice the steps.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testTwoTimeSteps() {
  const std::string filename = "VTKHDFTestTwoSteps";
  const std::string path     = filename + ".vtkhdf";

  {
    VTKHDFFileWriter plotter(".", 1);
    auto             v       = plotter.createVertexPlotter(0);
    auto             c       = plotter.createCellPlotter(0);
    const double     s       = 2.0 * thisRank();
    int              p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
    int              p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
    int              p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
    int              p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
    int              quad[4] = {p0, p1, p2, p3};
    c->plotQuadrangle(quad);
    // Two PlotSolution traversals append two steps to the same file.
    plotter.writeToFile(filename, 0.0);
    plotter.writeToFile(filename, 0.5);
  }

  barrier();
  if (isMaster()) {
    const int numberOfRanks = ranks();
    hid_t     file          = openForRead(path);

    check(readNSteps(file) == 2, "Two appends must give NSteps==2");
    const auto values = readF32(file, "/VTKHDF/Steps/Values");
    check(values.size() == 2 and values[0] == 0.0f and values[1] == 0.5f, "Steps/Values must be the two times");

    const auto pointOff = readI64(file, "/VTKHDF/Steps/PointOffsets");
    const auto cellOff  = readI64(file, "/VTKHDF/Steps/CellOffsets");
    const auto connOff  = readI64(file, "/VTKHDF/Steps/ConnectivityIdOffsets");
    const auto partOff  = readI64(file, "/VTKHDF/Steps/PartOffsets");
    check(
      pointOff.size() == 2 and pointOff[0] == 0 and pointOff[1] == 4 * numberOfRanks,
      "PointOffsets are cumulative prior-step points"
    );
    check(
      cellOff.size() == 2 and cellOff[0] == 0 and cellOff[1] == numberOfRanks,
      "CellOffsets are cumulative prior-step cells"
    );
    check(
      connOff.size() == 2 and connOff[0] == 0 and connOff[1] == 4 * numberOfRanks,
      "ConnectivityIdOffsets are cumulative prior-step connectivity entries"
    );
    check(
      partOff.size() == 2 and partOff[0] == 0 and partOff[1] == 1,
      "PartOffsets are cumulative parts (1 part/step)"
    );

    // Geometry arrays hold both steps concatenated.
    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    check(
      nPoints.size() == 2 and nPoints[0] == 4 * numberOfRanks and nPoints[1] == 4 * numberOfRanks,
      "Two per-step point totals"
    );

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Two subdomains are merged into a single part for the step, and the second
 * subdomain's connectivity is rebased past the first's points. Scales with the
 * MPI rank count.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testMultipleSubdomains() {
  const std::string filename = "VTKHDFTestMultiSubdomain";
  const std::string path     = filename + ".vtkhdf";

  // Each rank holds two subdomains: a triangle (3 pts) and a quad (4 pts). They
  // must merge into one part of 2 cells / 7 points, with the quad's connectivity
  // rebased past the triangle's 3 points.
  {
    VTKHDFFileWriter plotter(".", 2);
    const double     s = 10.0 * thisRank();
    {
      auto v      = plotter.createVertexPlotter(0);
      auto c      = plotter.createCellPlotter(0);
      int  p0     = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
      int  p1     = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
      int  p2     = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
      int  tri[3] = {p0, p1, p2};
      c->plotTriangle(tri);
    }
    {
      auto v       = plotter.createVertexPlotter(1);
      auto c       = plotter.createCellPlotter(1);
      int  p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 2.0, 0.0});
      int  p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 3.0, 0.0});
      int  p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 3.0, 1.0});
      int  p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 2.0, 1.0});
      int  quad[4] = {p0, p1, p2, p3};
      c->plotQuadrangle(quad);
    }
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int numberOfRanks = ranks();
    hid_t     file          = openForRead(path);

    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto nConn   = readI64(file, "/VTKHDF/NumberOfConnectivityIds");
    check(
      nPoints.size() == 1 and nPoints[0] == 7 * numberOfRanks,
      "Two subdomains merge to 7*ranks points (one entry)"
    );
    check(nCells.size() == 1 and nCells[0] == 2 * numberOfRanks, "Merged cell count is 2*ranks (single part)");
    check(nConn.size() == 1 and nConn[0] == 7 * numberOfRanks, "Merged connectivity is 7*ranks");

    const auto types = readU8AsInt(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == 2 * numberOfRanks, "Two cells per rank");
    for (int r = 0; r < numberOfRanks; r++) {
      check(types[2 * r] == 5, "First cell of each rank is a triangle (5)");
      check(types[2 * r + 1] == 9, "Second cell of each rank is a quad (9)");
    }

    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == 2 * numberOfRanks + 1, "Offsets holds cells+1 entries (single part)");
    for (int r = 0; r < numberOfRanks; r++) {
      check(offsets[2 * r] == 7 * r, "Before-offset of the rank's triangle");
      check(offsets[2 * r + 1] == 7 * r + 3, "Before-offset of the rank's quad, past the triangle's 3 nodes");
    }
    check(offsets[2 * numberOfRanks] == 7 * numberOfRanks, "Trailing sentinel equals the total connectivity");

    const auto conn = readI64(file, "/VTKHDF/Connectivity");
    for (int r = 0; r < numberOfRanks; r++) {
      // Triangle nodes [7r,7r+1,7r+2]; quad nodes [7r+3..7r+6] (rebased).
      check(conn[7 * r + 0] == 7 * r + 0, "Triangle node rebase");
      check(conn[7 * r + 3] == 7 * r + 3, "Quad node rebase past the triangle");
      check(conn[7 * r + 6] == 7 * r + 6, "Quad last node rebase");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A pure point cloud (vertices and vertex data, no cells), as produced by the
 * particle plotter: NumberOfCells==0 and Offsets==[0].
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testParticlePointCloud() {
  const std::string filename = "VTKHDFTestParticles";
  const std::string path     = filename + ".vtkhdf";

  // The particle plotter emits vertices + vertex data, no cells.
  {
    VTKHDFFileWriter plotter(".", 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "Mass", 1);
    int              a  = v->plotVertex(tarch::la::Vector<2, double>{0.5 + thisRank(), 0.5});
    vd->plotVertexData(a, 10.0);
    int b = v->plotVertex(tarch::la::Vector<2, double>{0.6 + thisRank(), 0.6});
    vd->plotVertexData(b, 20.0);
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int numberOfRanks = ranks();
    hid_t     file          = openForRead(path);

    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(nPoints.size() == 1 and nPoints[0] == 2 * numberOfRanks, "Point cloud has 2*ranks points");
    check(nCells.size() == 1 and nCells[0] == 0, "Point cloud has no cells");
    check(offsets.size() == 1 and offsets[0] == 0, "Offsets is the [0] sentinel alone for a cell-less grid");

    const auto mass = readF32(file, "/VTKHDF/PointData/Mass");
    check(static_cast<int>(mass.size()) == 2 * numberOfRanks, "One Mass value per point");

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A 3D hexahedron (VTK type 12) with cell data and non-zero z coordinates, which
 * covers the 3D geometry and connectivity path alongside the 2D tests.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::test3DHexahedron() {
  const std::string filename = "VTKHDFTest3DHex";
  const std::string path     = filename + ".vtkhdf";

  // Every rank contributes one unit cube (8 corners), shifted in x by 2*rank.
  {
    VTKHDFFileWriter plotter(".", 1);
    auto             v                = plotter.createVertexPlotter(0);
    auto             c                = plotter.createCellPlotter(0);
    auto             cd               = plotter.createCellDataPlotter(0, "rho", 1);
    const double     s                = 2.0 * thisRank();
    const double     cornerX[8]       = {0, 1, 1, 0, 0, 1, 1, 0};
    const double     cornerY[8]       = {0, 0, 1, 1, 0, 0, 1, 1};
    const double     cornerZ[8]       = {0, 0, 0, 0, 1, 1, 1, 1};
    int              cornerIndices[8] = {};
    for (int i = 0; i < 8; i++) {
      cornerIndices[i] = v->plotVertex(tarch::la::Vector<3, double>{s + cornerX[i], cornerY[i], cornerZ[i]});
    }
    int cell = c->plotHexahedron(cornerIndices);
    cd->plotCellData(cell, 2.5);
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int  numberOfRanks = ranks();
    hid_t      file          = openForRead(path);
    const auto nCells        = readI64(file, "/VTKHDF/NumberOfCells");
    const auto nPoints       = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto types         = readU8AsInt(file, "/VTKHDF/Types");
    const auto rho           = readF32(file, "/VTKHDF/CellData/rho");
    const auto points        = readF32(file, "/VTKHDF/Points");
    check(nCells.size() == 1 and nCells[0] == numberOfRanks, "One hexahedron per rank, merged");
    check(nPoints.size() == 1 and nPoints[0] == 8 * numberOfRanks, "Eight nodes per hexahedron");
    check(static_cast<int>(types.size()) == numberOfRanks, "One cell type per cell");
    for (int t : types) {
      check(t == 12, "VTK hexahedron type is 12");
    }
    check(static_cast<int>(rho.size()) == numberOfRanks, "One cell value per hexahedron");
    for (float r : rho) {
      check(r == 2.5f, "Hexahedron cell value round-trips");
    }
    check(static_cast<int>(points.size()) == 8 * numberOfRanks * 3, "3D points are (8*ranks, 3)");
    // The top face of the cube contributes the non-zero z coordinates.
    bool anyNonZeroZ = false;
    for (std::size_t i = 0; i < points.size() / 3; i++) {
      if (points[3 * i + 2] != 0.0f) {
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
 * Only the global master holds data; every other rank is empty. The collective
 * parallel write must not deadlock, and the merged grid carries only the
 * master's cells.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testHeterogeneousRanks() {
  const std::string filename = "VTKHDFTestHeterogeneous";
  const std::string path     = filename + ".vtkhdf";

  // Only the master holds data; the other ranks are empty. The collective write
  // must not deadlock and the merged grid must carry exactly the master's cell.
  {
    VTKHDFFileWriter plotter(".", 1);
    if (isMaster()) {
      auto v       = plotter.createVertexPlotter(0);
      auto c       = plotter.createCellPlotter(0);
      auto cd      = plotter.createCellDataPlotter(0, "Marker", 1);
      int  p0      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
      int  p1      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
      int  p2      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
      int  p3      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
      int  quad[4] = {p0, p1, p2, p3};
      int  cell    = c->plotQuadrangle(quad);
      cd->plotCellData(cell, 7.0);
    }
    plotter.writeToFile(filename, 0.0); // collective; must not deadlock
  }

  barrier();
  if (isMaster()) {
    hid_t      file    = openForRead(path);
    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto marker  = readF32(file, "/VTKHDF/CellData/Marker");
    check(nPoints.size() == 1 and nPoints[0] == 4, "Only the master's 4 points");
    check(nCells.size() == 1 and nCells[0] == 1, "Only the master's 1 cell");
    check(marker.size() == 1 and marker[0] == 7.0f, "The master's cell value survived");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Ranks hold different non-zero subdomain counts (master 3, the rest 2), as a
 * load-balanced distribution produces. A per-subdomain collective write loop
 * would mismatch the collective call count across ranks and deadlock.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "VTKHDFTestUneven";
  const std::string path     = filename + ".vtkhdf";

  // Master contributes 3 quads, every other rank 2 - non-zero but unequal counts.
  const int numberOfSubdomains = isMaster() ? 3 : 2;
  {
    VTKHDFFileWriter plotter(".", numberOfSubdomains);
    for (int sd = 0; sd < numberOfSubdomains; sd++) {
      auto         v       = plotter.createVertexPlotter(sd);
      auto         c       = plotter.createCellPlotter(sd);
      const double s       = 100.0 * thisRank() + 2.0 * sd;
      int          p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
      int          p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
      int          p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
      int          p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
      int          quad[4] = {p0, p1, p2, p3};
      c->plotQuadrangle(quad);
    }
    plotter.writeToFile(filename, 0.0); // uneven non-zero counts; must not deadlock
  }

  barrier();
  if (isMaster()) {
    const long long cells   = 3 + 2 * (ranks() - 1); // master 3, the other ranks 2 each
    hid_t           file    = openForRead(path);
    const auto      nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto      nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    check(
      nCells.size() == 1 and nCells[0] == cells,
      "Uneven distribution merges to one part with the summed cell total"
    );
    check(nPoints.size() == 1 and nPoints[0] == 4 * cells, "Uneven distribution merged node total");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void tarch::plotter::tests::VTKHDFFileWriterTest::testEmptyPlotter() {
  const std::string filename = "VTKHDFTestEmpty";
  const std::string path     = filename + ".vtkhdf";

  {
    VTKHDFFileWriter plotter(".", 1);
    plotter.writeToFile(filename, 0.0); // every rank empty; must not deadlock
  }

  barrier();
  if (isMaster()) {
    hid_t      file    = openForRead(path);
    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(readNSteps(file) == 1, "An empty write still records a step");
    check(nPoints.size() == 1 and nPoints[0] == 0, "Empty grid has 0 points");
    check(nCells.size() == 1 and nCells[0] == 0, "Empty grid has 0 cells");
    check(offsets.size() == 1 and offsets[0] == 0, "Empty grid Offsets is [0]");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void tarch::plotter::tests::VTKHDFFileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "VTKHDFTestInconsistent";

  bool thrown = false;
  try {
    VTKHDFFileWriter plotter(".", 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "Broken", 1);
    v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    vd->plotVertexData(0, 5.0); // data for only one of two vertices
    plotter.writeToFile(filename, 0.0);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  if (isMaster()) {
    removeFile(filename + ".vtkhdf");
  }
  check(thrown, "Inconsistent field size must be rejected on write");
}

void tarch::plotter::tests::VTKHDFFileWriterTest::testInvalidIdentifierThrows() {
  VTKHDFFileWriter plotter(".", 1);
  bool             thrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  check(thrown, "An illegal data identifier must be rejected");
}

/**
 * The precision switch selects the storage type: single-precision output stores
 * 4-byte reals, double-precision output stores 8-byte reals and round-trips a
 * value below single-precision resolution exactly.
 */
void tarch::plotter::tests::VTKHDFFileWriterTest::testPrecisionSwitch() {
  // The value differs from 1.0 by less than single precision resolves, so an
  // exact round-trip proves the data travelled in double precision.
  const double fineValue = 1.0 + 1.0e-12;

  auto writeQuadWithValue = [&](const std::string& filename) {
    VTKHDFFileWriter plotter(".", 1);
    auto             v       = plotter.createVertexPlotter(0);
    auto             c       = plotter.createCellPlotter(0);
    auto             cd      = plotter.createCellDataPlotter(0, "v", 1);
    const double     s       = 2.0 * thisRank();
    int              p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
    int              p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
    int              p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
    int              p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
    int              quad[4] = {p0, p1, p2, p3};
    int              cell    = c->plotQuadrangle(quad);
    cd->plotCellData(cell, fineValue);
    plotter.writeToFile(filename, 0.0);
  };

  auto storageBytes = [](hid_t file, const std::string& path) {
    hid_t             dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    hid_t             type    = H5Dget_type(dataset);
    const std::size_t bytes   = H5Tget_size(type);
    H5Tclose(type);
    H5Dclose(dataset);
    return bytes;
  };

  const std::string singleName = "VTKHDFTestSinglePrecision";
  const std::string doubleName = "VTKHDFTestDoublePrecision";

  check(not FileWriter::getDoublePrecisionOutput(), "Single precision is the default");
  writeQuadWithValue(singleName);

  FileWriter::setDoublePrecisionOutput(true);
  writeQuadWithValue(doubleName);
  FileWriter::setDoublePrecisionOutput(false);

  barrier();
  if (isMaster()) {
    hid_t singleFile = openForRead(singleName + ".vtkhdf");
    check(storageBytes(singleFile, "/VTKHDF/Points") == 4, "Single-precision output stores 4-byte points");
    check(storageBytes(singleFile, "/VTKHDF/CellData/v") == 4, "Single-precision output stores 4-byte fields");
    H5Fclose(singleFile);
    removeFile(singleName + ".vtkhdf");

    hid_t doubleFile = openForRead(doubleName + ".vtkhdf");
    check(storageBytes(doubleFile, "/VTKHDF/Points") == 8, "Double-precision output stores 8-byte points");
    check(storageBytes(doubleFile, "/VTKHDF/CellData/v") == 8, "Double-precision output stores 8-byte fields");

    hid_t   dataset   = H5Dopen(doubleFile, "/VTKHDF/CellData/v", H5P_DEFAULT);
    hid_t   dataSpace = H5Dget_space(dataset);
    hsize_t dims[2]   = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dims, nullptr);
    std::vector<double> values(dims[0]);
    H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    for (double value : values) {
      check(value == fineValue, "Double-precision output round-trips below single-precision resolution");
    }

    H5Fclose(doubleFile);
    removeFile(doubleName + ".vtkhdf");
  }
  barrier();
}

#else // No HDF5

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void tarch::plotter::tests::VTKHDFFileWriterTest::run() {}

#endif
