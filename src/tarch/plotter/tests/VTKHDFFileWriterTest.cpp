// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/plotter/VTKHDFFileWriter.h"

namespace fs = std::filesystem;

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() {
#ifdef PEANO_WITH_MPI
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
  }

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
    hid_t ds = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(ds >= 0, "missing dataset " + path);
    hid_t                  sp      = H5Dget_space(ds);
    hsize_t                dims[2] = {0, 0};
    const int              rank    = H5Sget_simple_extent_dims(sp, dims, nullptr);
    const hsize_t          n       = (rank == 2) ? dims[0] * dims[1] : dims[0];
    std::vector<long long> out(n);
    if (n > 0) {
      H5Dread(ds, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
    }
    H5Sclose(sp);
    H5Dclose(ds);
    return out;
  }

  std::vector<int> readU8AsInt(hid_t file, const std::string& path) {
    hid_t ds = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(ds >= 0, "missing dataset " + path);
    hid_t   sp      = H5Dget_space(ds);
    hsize_t dims[1] = {0};
    H5Sget_simple_extent_dims(sp, dims, nullptr);
    std::vector<unsigned char> raw(dims[0]);
    if (dims[0] > 0) {
      H5Dread(ds, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, raw.data());
    }
    H5Sclose(sp);
    H5Dclose(ds);
    return std::vector<int>(raw.begin(), raw.end());
  }

  std::vector<float> readF32(hid_t file, const std::string& path) {
    hid_t ds = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(ds >= 0, "missing dataset " + path);
    hid_t              sp      = H5Dget_space(ds);
    hsize_t            dims[2] = {0, 0};
    const int          rank    = H5Sget_simple_extent_dims(sp, dims, nullptr);
    const hsize_t      n       = (rank == 2) ? dims[0] * dims[1] : dims[0];
    std::vector<float> out(n);
    if (n > 0) {
      H5Dread(ds, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
    }
    H5Sclose(sp);
    H5Dclose(ds);
    return out;
  }

  long long readNSteps(hid_t file) {
    hid_t     g  = H5Gopen(file, "/VTKHDF/Steps", H5P_DEFAULT);
    hid_t     a  = H5Aopen(g, "NSteps", H5P_DEFAULT);
    long long ns = 0;
    H5Aread(a, H5T_NATIVE_LLONG, &ns);
    H5Aclose(a);
    H5Gclose(g);
    return ns;
  }

  hid_t openForRead(const std::string& path) {
    hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    check(file >= 0, "could not reopen " + path);
    return file;
  }

  void removeFile(const std::string& path) {
    if (fs::exists(path)) {
      fs::remove(path);
    }
  }
} // namespace

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void tarch::plotter::tests::VTKHDFFileWriterTest::run() {
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
}

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
    const int R    = ranks();
    hid_t     file = openForRead(path);

    check(readNSteps(file) == 1, "NSteps should be 1");
    const auto        nPoints     = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto        nCells      = readI64(file, "/VTKHDF/NumberOfCells");
    const auto        nConn       = readI64(file, "/VTKHDF/NumberOfConnectivityIds");
    const std::string nPointsInfo = "size=" + std::to_string(nPoints.size()) +
                                    " value=" + (nPoints.empty() ? std::string("none") : std::to_string(nPoints[0]));
    check(
      nPoints.size() == 1 and nPoints[0] == 4 * R,
      "NumberOfPoints must be one per-step total 4*ranks (R=" + std::to_string(R) + ", got " + nPointsInfo + ")"
    );
    check(nCells.size() == 1 and nCells[0] == R, "NumberOfCells must be one per-step total ranks (single part)");
    check(nConn.size() == 1 and nConn[0] == 4 * R, "NumberOfConnectivityIds total must be 4*ranks");

    const auto types = readU8AsInt(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == R, "Types one per cell");
    for (int t : types) {
      check(t == 9, "every cell must be a VTK quad (9)");
    }

    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == R + 1, "Offsets must have cells+1 entries (single part)");
    for (int i = 0; i <= R; i++) {
      check(offsets[i] == 4 * i, "Offsets must be the global cumulative 0,4,8,...");
    }

    const auto conn = readI64(file, "/VTKHDF/Connectivity");
    check(static_cast<int>(conn.size()) == 4 * R, "Connectivity total");
    for (int r = 0; r < R; r++) {
      for (int k = 0; k < 4; k++) {
        check(conn[4 * r + k] == 4 * r + k, "connectivity must be rebased to global point indices per rank");
      }
    }

    const auto cellData = readF32(file, "/VTKHDF/CellData/v");
    check(static_cast<int>(cellData.size()) == R, "one cell value per cell");
    for (int r = 0; r < R; r++) {
      check(cellData[r] == static_cast<float>(r), "cell value must equal the contributing rank id, in rank order");
    }

    const auto points = readF32(file, "/VTKHDF/Points");
    check(static_cast<int>(points.size()) == 4 * R * 3, "Points are (4*ranks, 3)");
    for (int r = 0; r < R; r++) {
      // first vertex of rank r's quad: x == 2*r, y == 0, z == 0
      check(points[(4 * r) * 3 + 0] == static_cast<float>(2 * r), "rank quad x-origin");
      check(points[(4 * r) * 3 + 2] == 0.0f, "2D points get z==0");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

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
    const int R    = ranks();
    hid_t     file = openForRead(path);

    check(readNSteps(file) == 2, "two appends must give NSteps==2");
    const auto values = readF32(file, "/VTKHDF/Steps/Values");
    check(values.size() == 2 and values[0] == 0.0f and values[1] == 0.5f, "Steps/Values must be the two times");

    const auto pointOff = readI64(file, "/VTKHDF/Steps/PointOffsets");
    const auto cellOff  = readI64(file, "/VTKHDF/Steps/CellOffsets");
    const auto connOff  = readI64(file, "/VTKHDF/Steps/ConnectivityIdOffsets");
    const auto partOff  = readI64(file, "/VTKHDF/Steps/PartOffsets");
    check(
      pointOff.size() == 2 and pointOff[0] == 0 and pointOff[1] == 4 * R,
      "PointOffsets are cumulative prior-step points"
    );
    check(cellOff.size() == 2 and cellOff[0] == 0 and cellOff[1] == R, "CellOffsets are cumulative prior-step cells");
    check(
      connOff.size() == 2 and connOff[0] == 0 and connOff[1] == 4 * R,
      "ConnectivityIdOffsets cumulative prior-step conn"
    );
    check(
      partOff.size() == 2 and partOff[0] == 0 and partOff[1] == 1,
      "PartOffsets are cumulative parts (1 part/step)"
    );

    // Geometry arrays hold both steps concatenated.
    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    check(nPoints.size() == 2 and nPoints[0] == 4 * R and nPoints[1] == 4 * R, "two per-step point totals");

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

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
    const int R    = ranks();
    hid_t     file = openForRead(path);

    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto nConn   = readI64(file, "/VTKHDF/NumberOfConnectivityIds");
    check(nPoints.size() == 1 and nPoints[0] == 7 * R, "two subdomains merge to 7*ranks points (one entry)");
    check(nCells.size() == 1 and nCells[0] == 2 * R, "merged cell count 2*ranks (single part)");
    check(nConn.size() == 1 and nConn[0] == 7 * R, "merged connectivity 7*ranks");

    const auto types = readU8AsInt(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == 2 * R, "two cells per rank");
    for (int r = 0; r < R; r++) {
      check(types[2 * r] == 5, "first cell of each rank is a triangle (5)");
      check(types[2 * r + 1] == 9, "second cell of each rank is a quad (9)");
    }

    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(static_cast<int>(offsets.size()) == 2 * R + 1, "Offsets cells+1 (single part)");
    for (int r = 0; r < R; r++) {
      check(offsets[2 * r] == 7 * r, "before-offset of rank r's triangle");
      check(offsets[2 * r + 1] == 7 * r + 3, "before-offset of rank r's quad (past triangle's 3 nodes)");
    }
    check(offsets[2 * R] == 7 * R, "trailing sentinel == total connectivity");

    const auto conn = readI64(file, "/VTKHDF/Connectivity");
    for (int r = 0; r < R; r++) {
      // triangle nodes [7r,7r+1,7r+2]; quad nodes [7r+3..7r+6] (rebased)
      check(conn[7 * r + 0] == 7 * r + 0, "triangle node rebase");
      check(conn[7 * r + 3] == 7 * r + 3, "quad node rebase past triangle");
      check(conn[7 * r + 6] == 7 * r + 6, "quad last node rebase");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

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
    const int R    = ranks();
    hid_t     file = openForRead(path);

    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto offsets = readI64(file, "/VTKHDF/Offsets");
    check(nPoints.size() == 1 and nPoints[0] == 2 * R, "point cloud has 2*ranks points");
    check(nCells.size() == 1 and nCells[0] == 0, "point cloud has no cells");
    check(offsets.size() == 1 and offsets[0] == 0, "Offsets is just the [0] sentinel for a cell-less grid");

    const auto mass = readF32(file, "/VTKHDF/PointData/Mass");
    check(static_cast<int>(mass.size()) == 2 * R, "one Mass per point");

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void tarch::plotter::tests::VTKHDFFileWriterTest::test3DHexahedron() {
  const std::string filename = "VTKHDFTest3DHex";
  const std::string path     = filename + ".vtkhdf";

  // Every rank contributes one unit cube (8 corners), shifted in x by 2*rank.
  {
    VTKHDFFileWriter plotter(".", 1);
    auto             v    = plotter.createVertexPlotter(0);
    auto             c    = plotter.createCellPlotter(0);
    auto             cd   = plotter.createCellDataPlotter(0, "rho", 1);
    const double     s    = 2.0 * thisRank();
    const double     X[8] = {0, 1, 1, 0, 0, 1, 1, 0};
    const double     Y[8] = {0, 0, 1, 1, 0, 0, 1, 1};
    const double     Z[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    int              idx[8];
    for (int i = 0; i < 8; i++) {
      idx[i] = v->plotVertex(tarch::la::Vector<3, double>{s + X[i], Y[i], Z[i]});
    }
    int cell = c->plotHexahedron(idx);
    cd->plotCellData(cell, 2.5);
    plotter.writeToFile(filename, 0.0);
  }

  barrier();
  if (isMaster()) {
    const int  R       = ranks();
    hid_t      file    = openForRead(path);
    const auto nCells  = readI64(file, "/VTKHDF/NumberOfCells");
    const auto nPoints = readI64(file, "/VTKHDF/NumberOfPoints");
    const auto types   = readU8AsInt(file, "/VTKHDF/Types");
    const auto rho     = readF32(file, "/VTKHDF/CellData/rho");
    const auto points  = readF32(file, "/VTKHDF/Points");
    check(nCells.size() == 1 and nCells[0] == R, "one hexahedron per rank, merged");
    check(nPoints.size() == 1 and nPoints[0] == 8 * R, "eight nodes per hexahedron");
    check(static_cast<int>(types.size()) == R, "one cell type per cell");
    for (int t : types) {
      check(t == 12, "VTK hexahedron type is 12");
    }
    check(static_cast<int>(rho.size()) == R, "one cell value per hexahedron");
    for (float r : rho) {
      check(r == 2.5f, "hexahedron cell value round-trips");
    }
    check(static_cast<int>(points.size()) == 8 * R * 3, "3D points are (8*ranks, 3)");
    // a non-zero z coordinate must be present (the top face of the cube)
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
    check(nPoints.size() == 1 and nPoints[0] == 4, "only the master's 4 points");
    check(nCells.size() == 1 and nCells[0] == 1, "only the master's 1 cell");
    check(marker.size() == 1 and marker[0] == 7.0f, "the master's cell value survived");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

void tarch::plotter::tests::VTKHDFFileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "VTKHDFTestUneven";
  const std::string path     = filename + ".vtkhdf";

  // Master contributes 3 quads, every other rank 2 - non-zero but unequal counts.
  const int nSub = isMaster() ? 3 : 2;
  {
    VTKHDFFileWriter plotter(".", nSub);
    for (int sd = 0; sd < nSub; sd++) {
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
      "uneven distribution merges to one part with the summed cell total"
    );
    check(nPoints.size() == 1 and nPoints[0] == 4 * cells, "uneven distribution merged node total");
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
    check(readNSteps(file) == 1, "empty write still records a step");
    check(nPoints.size() == 1 and nPoints[0] == 0, "empty grid: 0 points");
    check(nCells.size() == 1 and nCells[0] == 0, "empty grid: 0 cells");
    check(offsets.size() == 1 and offsets[0] == 0, "empty grid Offsets is [0]");
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
  check(thrown, "inconsistent field size must be rejected on write");
}

void tarch::plotter::tests::VTKHDFFileWriterTest::testInvalidIdentifierThrows() {
  VTKHDFFileWriter plotter(".", 1);
  bool             thrown = false;
  try {
    plotter.createCellDataPlotter(0, "bad/name", 1);
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  check(thrown, "an illegal data identifier must be rejected");
}

#else // No HDF5

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void tarch::plotter::tests::VTKHDFFileWriterTest::run() {}

#endif
