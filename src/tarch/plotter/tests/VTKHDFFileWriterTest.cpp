// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriterTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/VTKHDFFileWriter.h"
#include "tarch/plotter/tests/Meshes.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  constexpr auto LineType       = static_cast<std::uint8_t>(tarch::plotter::CellType::Line);
  constexpr auto QuadrangleType = static_cast<std::uint8_t>(tarch::plotter::CellType::Quadrangle);
  constexpr auto HexahedronType = static_cast<std::uint8_t>(tarch::plotter::CellType::Hexahedron);

  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  int getNumberOfRanks() { return tarch::mpi::Rank::getInstance().getNumberOfRanks(); }
  int getRank() { return tarch::mpi::Rank::getInstance().getRank(); }
  bool isMaster() { return tarch::mpi::Rank::getInstance().isGlobalMaster(); }

  void check(const bool condition, const std::string& message) {
    if (not condition) {
      throw std::runtime_error(message);
    }
  }

  // The global master calls the read-back helpers after the collective write. Each helper reads a
  // whole dataset.
  std::vector<long long> readInt64Dataset(const hid_t file, const std::string& path) {
    const hid_t dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    check(dataset >= 0, "The file does not contain the dataset " + path + ".");
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
    check(dataset >= 0, "The file does not contain the dataset " + path + ".");
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
    check(dataset >= 0, "The file does not contain the dataset " + path + ".");
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
    check(file >= 0, "HDF5 cannot open the file " + path + ".");
    return file;
  }

  void removeFile(const std::string& path) {
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
  }

  void writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::VTKHDFFileWriter writer;
    writer.write(mesh, ".", filename, 0.0);
  }
} // namespace

tarch::plotter::tests::VTKHDFFileWriterTest::VTKHDFFileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::VTKHDFFileWriterTest") {}

void
tarch::plotter::tests::VTKHDFFileWriterTest::run() {
  // The function check() and the read-back helpers throw for a malformed file. An exception that
  // leaves run() ends the test executable.
  try {
    testMethod(testSingleQuadrangle2D);
    testMethod(testTwoTimeSteps);
    testMethod(testMixedCellTypes);
    testMethod(testParticlePointCloud);
    testMethod(test3DHexahedron);
    testMethod(testHeterogeneousRanks);
    testMethod(testUnevenCellCounts);
    testMethod(testEmptyMesh);
    testMethod(testPrecisionSwitch);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

/**
 * The file of one step lists the quadrangles of all ranks in one part.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testSingleQuadrangle2D() {
  const std::string filename = "VTKHDFTestSingleQuadrangle";
  const std::string path     = filename + ".vtkhdf";

  // Every rank shifts its quadrangle in x by twice its rank number. The connectivity of rank r refers
  // to the points 4r to 4r+3 of the merged part.
  {
    tarch::plotter::Mesh mesh;
    appendUnitSquares(mesh, 2.0 * getRank(), 1);
    mesh.cellFields.push_back({"v", 1, {static_cast<double>(getRank())}});
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    check(readNumberOfSteps(file) == 1, "The attribute NSteps differs from 1.");
    // A reader rejects a file whose version is newer than the version it supports.
    const auto version = readVersion(file);
    check(version[0] == 2 and version[1] == 0, "The attribute Version differs from 2.0.");
    const auto numberOfPoints          = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto numberOfCells           = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto numberOfConnectivityIds = readInt64Dataset(file, "/VTKHDF/NumberOfConnectivityIds");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 4 * numberOfRanks,
      "The dataset NumberOfPoints does not count four points per rank in one entry."
    );
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == numberOfRanks,
      "The dataset NumberOfCells does not count one cell per rank in one entry."
    );
    check(
      numberOfConnectivityIds.size() == 1 and numberOfConnectivityIds[0] == 4 * numberOfRanks,
      "The dataset NumberOfConnectivityIds does not count four corners per rank in one entry."
    );

    const auto types = readUnsignedByteDataset(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == numberOfRanks, "The dataset Types does not list one type per cell.");
    for (const int type : types) {
      check(type == QuadrangleType, "The dataset Types lists a cell type that differs from the quadrangle.");
    }

    const auto offsets = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(
      static_cast<int>(offsets.size()) == numberOfRanks + 1,
      "The dataset Offsets does not list one entry per cell and one more."
    );
    for (int cellIndex = 0; cellIndex <= numberOfRanks; cellIndex++) {
      check(offsets[cellIndex] == 4 * cellIndex, "The dataset Offsets differs from the sequence 0, 4, 8 and so on.");
    }

    const auto connectivity = readInt64Dataset(file, "/VTKHDF/Connectivity");
    check(
      static_cast<int>(connectivity.size()) == 4 * numberOfRanks,
      "The dataset Connectivity does not list four corners per rank."
    );
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      for (int nodeIndex = 0; nodeIndex < 4; nodeIndex++) {
        check(
          connectivity[4 * rankIndex + nodeIndex] == 4 * rankIndex + nodeIndex,
          "The dataset Connectivity does not number the points of every rank after the points of the ranks before."
        );
      }
    }

    const auto cellData = readFloat32Dataset(file, "/VTKHDF/CellData/v");
    check(
      static_cast<int>(cellData.size()) == numberOfRanks,
      "The dataset CellData/v does not list one value per cell."
    );
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(
        cellData[rankIndex] == static_cast<float>(rankIndex),
        "The dataset CellData/v does not list the rank numbers in rank order."
      );
    }

    const auto points = readFloat32Dataset(file, "/VTKHDF/Points");
    check(
      static_cast<int>(points.size()) == 4 * numberOfRanks * 3,
      "The dataset Points does not list four points per rank."
    );
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(
        points[(4 * rankIndex) * 3 + 0] == static_cast<float>(2 * rankIndex),
        "The first point of a rank differs from the x offset of the rank."
      );
      check(points[(4 * rankIndex) * 3 + 2] == 0.0F, "The z coordinate of a 2D point differs from zero.");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * Every write appends a step whose offsets in Steps count the entries of the preceding steps.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testTwoTimeSteps() {
  const std::string filename = "VTKHDFTestTwoSteps";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::Mesh mesh;
    appendUnitSquares(mesh, 2.0 * getRank(), 1);
    tarch::plotter::VTKHDFFileWriter writer;
    writer.write(mesh, ".", filename, 0.0);
    writer.write(mesh, ".", filename, 0.5);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    check(readNumberOfSteps(file) == 2, "The attribute NSteps differs from 2 after two writes.");
    const auto values = readFloat32Dataset(file, "/VTKHDF/Steps/Values");
    check(
      values.size() == 2 and values[0] == 0.0F and values[1] == 0.5F,
      "The dataset Steps/Values differs from the times 0 and 0.5."
    );

    const auto pointOffsets        = readInt64Dataset(file, "/VTKHDF/Steps/PointOffsets");
    const auto cellOffsets         = readInt64Dataset(file, "/VTKHDF/Steps/CellOffsets");
    const auto connectivityOffsets = readInt64Dataset(file, "/VTKHDF/Steps/ConnectivityIdOffsets");
    const auto partOffsets         = readInt64Dataset(file, "/VTKHDF/Steps/PartOffsets");
    check(
      pointOffsets.size() == 2 and pointOffsets[0] == 0 and pointOffsets[1] == 4 * numberOfRanks,
      "The dataset Steps/PointOffsets does not count the points of the steps before."
    );
    check(
      cellOffsets.size() == 2 and cellOffsets[0] == 0 and cellOffsets[1] == numberOfRanks,
      "The dataset Steps/CellOffsets does not count the cells of the steps before."
    );
    check(
      connectivityOffsets.size() == 2 and connectivityOffsets[0] == 0 and connectivityOffsets[1] == 4 * numberOfRanks,
      "The dataset Steps/ConnectivityIdOffsets does not count the corners of the steps before."
    );
    check(
      partOffsets.size() == 2 and partOffsets[0] == 0 and partOffsets[1] == 1,
      "The dataset Steps/PartOffsets does not count one part per step before."
    );

    // The geometry arrays hold both steps one after the other.
    const auto numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    check(
      numberOfPoints.size() == 2 and numberOfPoints[0] == 4 * numberOfRanks and numberOfPoints[1] == 4 * numberOfRanks,
      "The dataset NumberOfPoints does not count four points per rank for both steps."
    );

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * The file stores a line and a quadrangle of every rank in one part of the step.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testMixedCellTypes() {
  const std::string filename = "VTKHDFTestMixedCellTypes";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::Mesh mesh;
    const double         x = 10.0 * getRank();
    mesh.pointsXYZ         = {x, 0.0, 0.0, x + 1.0, 0.0, 0.0};
    mesh.connectivity      = {0, 1};
    mesh.offsets           = {2};
    mesh.types             = {LineType};
    appendUnitSquares(mesh, x + 2.0, 1);
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    const auto numberOfPoints          = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto numberOfCells           = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto numberOfConnectivityIds = readInt64Dataset(file, "/VTKHDF/NumberOfConnectivityIds");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 6 * numberOfRanks,
      "The dataset NumberOfPoints does not count six points per rank in one entry."
    );
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == 2 * numberOfRanks,
      "The dataset NumberOfCells does not count two cells per rank in one entry."
    );
    check(
      numberOfConnectivityIds.size() == 1 and numberOfConnectivityIds[0] == 6 * numberOfRanks,
      "The dataset NumberOfConnectivityIds does not count six corners per rank in one entry."
    );

    const auto types = readUnsignedByteDataset(file, "/VTKHDF/Types");
    check(static_cast<int>(types.size()) == 2 * numberOfRanks, "The dataset Types does not list two cells per rank.");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(types[2 * rankIndex] == LineType, "The first cell of a rank differs from a line.");
      check(types[2 * rankIndex + 1] == QuadrangleType, "The second cell of a rank differs from a quadrangle.");
    }

    const auto offsets = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(
      static_cast<int>(offsets.size()) == 2 * numberOfRanks + 1,
      "The dataset Offsets does not list one entry per cell and one more."
    );
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      check(
        offsets[2 * rankIndex] == 6 * rankIndex,
        "The line of a rank does not start after the corners of the ranks before."
      );
      check(offsets[2 * rankIndex + 1] == 6 * rankIndex + 2, "The quadrangle of a rank does not start after the line.");
    }
    check(
      offsets[2 * numberOfRanks] == 6 * numberOfRanks,
      "The last entry of Offsets differs from the number of corners."
    );

    const auto connectivity = readInt64Dataset(file, "/VTKHDF/Connectivity");
    for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
      // The line refers to the points 6r and 6r+1 and the quadrangle to the points 6r+2 to 6r+5.
      check(connectivity[6 * rankIndex + 0] == 6 * rankIndex + 0, "The line of a rank refers to a wrong point.");
      check(connectivity[6 * rankIndex + 2] == 6 * rankIndex + 2, "The quadrangle of a rank refers to a wrong point.");
      check(connectivity[6 * rankIndex + 5] == 6 * rankIndex + 5, "The quadrangle of a rank refers to a wrong point.");
    }

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * The file of points without cells lists zero cells and the single offset 0.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testParticlePointCloud() {
  const std::string filename = "VTKHDFTestParticles";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ = {0.5 + getRank(), 0.5, 0.0, 0.6 + getRank(), 0.6, 0.0};
    mesh.pointFields.push_back({"Mass", 1, {10.0, 20.0}});
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks = getNumberOfRanks();
    const hid_t file          = openForRead(path);

    const auto numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto offsets        = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 2 * numberOfRanks,
      "The dataset NumberOfPoints does not count two points per rank in one entry."
    );
    check(numberOfCells.size() == 1 and numberOfCells[0] == 0, "The dataset NumberOfCells differs from zero.");
    check(offsets.size() == 1 and offsets[0] == 0, "The dataset Offsets differs from the single entry 0.");

    const auto mass = readFloat32Dataset(file, "/VTKHDF/PointData/Mass");
    check(
      static_cast<int>(mass.size()) == 2 * numberOfRanks,
      "The dataset PointData/Mass does not list one value per point."
    );

    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * The file stores the points, the type and the cell value of one hexahedron per rank.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::test3DHexahedron() {
  const std::string filename = "VTKHDFTest3DHexahedron";
  const std::string path     = filename + ".vtkhdf";

  // Every rank shifts its unit cube in x by twice its rank number.
  {
    tarch::plotter::Mesh mesh;
    appendUnitCube(mesh, 2.0 * getRank());
    mesh.cellFields.push_back({"Density", 1, {2.5}});
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const int   numberOfRanks  = getNumberOfRanks();
    const hid_t file           = openForRead(path);
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  types          = readUnsignedByteDataset(file, "/VTKHDF/Types");
    const auto  density        = readFloat32Dataset(file, "/VTKHDF/CellData/Density");
    const auto  points         = readFloat32Dataset(file, "/VTKHDF/Points");
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == numberOfRanks,
      "The dataset NumberOfCells does not count one cell per rank in one entry."
    );
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 8 * numberOfRanks,
      "The dataset NumberOfPoints does not count eight points per rank in one entry."
    );
    check(static_cast<int>(types.size()) == numberOfRanks, "The dataset Types does not list one type per cell.");
    for (const int type : types) {
      check(type == HexahedronType, "The dataset Types lists a cell type that differs from the hexahedron.");
    }
    check(
      static_cast<int>(density.size()) == numberOfRanks,
      "The dataset CellData/Density does not list one value per cell."
    );
    for (const float value : density) {
      check(value == 2.5F, "The dataset CellData/Density differs from 2.5.");
    }
    check(
      static_cast<int>(points.size()) == 8 * numberOfRanks * 3,
      "The dataset Points does not list eight points per rank."
    );
    // The top face of every cube lies at z = 1.
    bool anyNonZeroZ = false;
    for (std::size_t pointIndex = 0; pointIndex < points.size() / 3; pointIndex++) {
      if (points[3 * pointIndex + 2] != 0.0F) {
        anyNonZeroZ = true;
      }
    }
    check(anyNonZeroZ, "Every z coordinate of the dataset Points equals zero.");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A collective write with cells on the global master alone writes the cells of the master.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testHeterogeneousRanks() {
  const std::string filename = "VTKHDFTestHeterogeneous";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::Mesh mesh;
    if (isMaster()) {
      appendUnitSquares(mesh, 0.0, 1);
      mesh.cellFields.push_back({"Marker", 1, {7.0}});
    } else {
      mesh.cellFields.push_back({"Marker", 1, {}});
    }
    // Every rank enters this collective call.
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const hid_t file           = openForRead(path);
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  marker         = readFloat32Dataset(file, "/VTKHDF/CellData/Marker");
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 4,
      "The dataset NumberOfPoints differs from the four points of the global master."
    );
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == 1,
      "The dataset NumberOfCells differs from the one cell of the global master."
    );
    check(
      marker.size() == 1 and marker[0] == 7.0F,
      "The dataset CellData/Marker differs from the value 7 of the global master."
    );
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * The collective write completes for three cells on the global master and two on every other rank.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testUnevenCellCounts() {
  const std::string filename = "VTKHDFTestUneven";
  const std::string path     = filename + ".vtkhdf";

  {
    tarch::plotter::Mesh mesh;
    appendUnitSquares(mesh, 100.0 * getRank(), isMaster() ? 3 : 2);
    writeMesh(mesh, filename);
  }

  barrier();
  if (isMaster()) {
    const long long expectedNumberOfCells = 3 + 2 * (getNumberOfRanks() - 1);
    const hid_t     file                  = openForRead(path);
    const auto      numberOfCells         = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto      numberOfPoints        = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    check(
      numberOfCells.size() == 1 and numberOfCells[0] == expectedNumberOfCells,
      "The dataset NumberOfCells does not count the cells of all ranks in one entry."
    );
    check(
      numberOfPoints.size() == 1 and numberOfPoints[0] == 4 * expectedNumberOfCells,
      "The dataset NumberOfPoints does not count the points of all ranks in one entry."
    );
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * A mesh without points, cells and fields gives a valid file with an empty grid.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testEmptyMesh() {
  const std::string filename = "VTKHDFTestEmpty";
  const std::string path     = filename + ".vtkhdf";

  // Every rank enters this collective call with an empty grid.
  writeMesh(tarch::plotter::Mesh(), filename);

  barrier();
  if (isMaster()) {
    const hid_t file           = openForRead(path);
    const auto  numberOfPoints = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
    const auto  numberOfCells  = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
    const auto  offsets        = readInt64Dataset(file, "/VTKHDF/Offsets");
    check(readNumberOfSteps(file) == 1, "The attribute NSteps differs from 1 after the write of an empty mesh.");
    check(numberOfPoints.size() == 1 and numberOfPoints[0] == 0, "The dataset NumberOfPoints differs from zero.");
    check(numberOfCells.size() == 1 and numberOfCells[0] == 0, "The dataset NumberOfCells differs from zero.");
    check(offsets.size() == 1 and offsets[0] == 0, "The dataset Offsets differs from the single entry 0.");
    H5Fclose(file);
    removeFile(path);
  }
  barrier();
}

/**
 * The function setDoublePrecisionOutput() selects 4-byte or 8-byte reals for the points and the
 * fields.
 */
void
tarch::plotter::tests::VTKHDFFileWriterTest::testPrecisionSwitch() {
  // fineValue differs from 1.0 by less than the resolution of single precision.
  const double fineValue = 1.0 + 1.0e-12;

  const auto writeQuadrangleWithValue = [&](const std::string& filename) {
    tarch::plotter::Mesh mesh;
    appendUnitSquares(mesh, 2.0 * getRank(), 1);
    mesh.cellFields.push_back({"v", 1, {fineValue}});
    writeMesh(mesh, filename);
  };

  const auto getStorageBytes = [](const hid_t file, const std::string& path) {
    const hid_t       dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    const hid_t       type    = H5Dget_type(dataset);
    const std::size_t bytes   = H5Tget_size(type);
    H5Tclose(type);
    H5Dclose(dataset);
    return bytes;
  };

  const std::string singleName = "VTKHDFTestSinglePrecision";
  const std::string doubleName = "VTKHDFTestDoublePrecision";

  check(not tarch::plotter::FileWriter::getDoublePrecisionOutput(), "FileWriter writes double precision by default.");
  writeQuadrangleWithValue(singleName);

  tarch::plotter::FileWriter::setDoublePrecisionOutput(true);
  writeQuadrangleWithValue(doubleName);
  tarch::plotter::FileWriter::setDoublePrecisionOutput(false);

  barrier();
  if (isMaster()) {
    const hid_t singleFile = openForRead(singleName + ".vtkhdf");
    check(
      getStorageBytes(singleFile, "/VTKHDF/Points") == 4,
      "The dataset Points of the single-precision file differs from 4 bytes per real."
    );
    check(
      getStorageBytes(singleFile, "/VTKHDF/CellData/v") == 4,
      "The dataset CellData/v of the single-precision file differs from 4 bytes per real."
    );
    H5Fclose(singleFile);
    removeFile(singleName + ".vtkhdf");

    const hid_t doubleFile = openForRead(doubleName + ".vtkhdf");
    check(
      getStorageBytes(doubleFile, "/VTKHDF/Points") == 8,
      "The dataset Points of the double-precision file differs from 8 bytes per real."
    );
    check(
      getStorageBytes(doubleFile, "/VTKHDF/CellData/v") == 8,
      "The dataset CellData/v of the double-precision file differs from 8 bytes per real."
    );

    const hid_t dataset       = H5Dopen(doubleFile, "/VTKHDF/CellData/v", H5P_DEFAULT);
    const hid_t dataSpace     = H5Dget_space(dataset);
    hsize_t     dimensions[2] = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    std::vector<double> values(dimensions[0]);
    H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    for (const double value : values) {
      check(value == fineValue, "The dataset CellData/v of the double-precision file differs from the written value.");
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
