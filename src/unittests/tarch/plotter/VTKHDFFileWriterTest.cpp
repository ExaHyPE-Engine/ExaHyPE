// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/VTKHDFFileWriter.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/Mesh.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/plotter/Meshes.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  constexpr int LineType       = static_cast<int>(tarch::plotter::CellType::Line);
  constexpr int QuadrangleType = static_cast<int>(tarch::plotter::CellType::Quadrangle);
  constexpr int HexahedronType = static_cast<int>(tarch::plotter::CellType::Hexahedron);

  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  int getNumberOfRanks() { return tarch::mpi::Rank::getInstance().getNumberOfRanks(); }
  int getRank() { return tarch::mpi::Rank::getInstance().getRank(); }
  bool isMaster() { return tarch::mpi::Rank::getInstance().isGlobalMaster(); }

  std::string getPath(const std::string& filename) {
    return (tarch::testing::getWritableTestDirectory() / (filename + ".vtkhdf")).string();
  }

  // The global master calls the read-back helpers once the collective write has closed the file.
  // Each helper reads a whole dataset.
  std::vector<long long> readInt64Dataset(const hid_t file, const std::string& path) {
    const hid_t            dataset         = H5Dopen(file, path.c_str(), H5P_DEFAULT);
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
    const hid_t dataset       = H5Dopen(file, path.c_str(), H5P_DEFAULT);
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
    const hid_t        dataset         = H5Dopen(file, path.c_str(), H5P_DEFAULT);
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

  std::size_t getStorageBytes(const hid_t file, const std::string& path) {
    const hid_t       dataset = H5Dopen(file, path.c_str(), H5P_DEFAULT);
    const hid_t       type    = H5Dget_type(dataset);
    const std::size_t bytes   = H5Tget_size(type);
    H5Tclose(type);
    H5Dclose(dataset);
    return bytes;
  }

  /**
   * The VTKHDF UnstructuredGrid layout that a reader needs to slice each step, as the global master
   * read it back through the HDF5 C API. The other ranks keep the empty defaults.
   */
  struct ReadBackGrid final {
    bool                   opened{false};
    long long              numberOfSteps{0};
    std::vector<long long> version{};
    std::vector<long long> numberOfPoints{};
    std::vector<long long> numberOfCells{};
    std::vector<long long> numberOfConnectivityIds{};
    std::vector<int>       types{};
    std::vector<long long> offsets{};
    std::vector<long long> connectivity{};
    std::vector<float>     points{};
    std::vector<float>     stepValues{};
    std::vector<long long> pointOffsets{};
    std::vector<long long> cellOffsets{};
    std::vector<long long> connectivityOffsets{};
    std::vector<long long> partOffsets{};
    std::vector<float>     fieldValues{};
  };

  /**
   * Reads the grid and the field of fieldPath on the global master and deletes the file. An empty
   * fieldPath skips the field.
   */
  ReadBackGrid readAndRemoveOnGlobalMaster(const std::string& filename, const std::string& fieldPath) {
    ReadBackGrid grid;
    barrier();
    if (isMaster()) {
      const hid_t file = H5Fopen(getPath(filename).c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      grid.opened      = file >= 0;
      if (grid.opened) {
        grid.numberOfSteps           = readNumberOfSteps(file);
        grid.version                 = readVersion(file);
        grid.numberOfPoints          = readInt64Dataset(file, "/VTKHDF/NumberOfPoints");
        grid.numberOfCells           = readInt64Dataset(file, "/VTKHDF/NumberOfCells");
        grid.numberOfConnectivityIds = readInt64Dataset(file, "/VTKHDF/NumberOfConnectivityIds");
        grid.types                   = readUnsignedByteDataset(file, "/VTKHDF/Types");
        grid.offsets                 = readInt64Dataset(file, "/VTKHDF/Offsets");
        grid.connectivity            = readInt64Dataset(file, "/VTKHDF/Connectivity");
        grid.points                  = readFloat32Dataset(file, "/VTKHDF/Points");
        grid.stepValues              = readFloat32Dataset(file, "/VTKHDF/Steps/Values");
        grid.pointOffsets            = readInt64Dataset(file, "/VTKHDF/Steps/PointOffsets");
        grid.cellOffsets             = readInt64Dataset(file, "/VTKHDF/Steps/CellOffsets");
        grid.connectivityOffsets     = readInt64Dataset(file, "/VTKHDF/Steps/ConnectivityIdOffsets");
        grid.partOffsets             = readInt64Dataset(file, "/VTKHDF/Steps/PartOffsets");
        if (not fieldPath.empty()) {
          grid.fieldValues = readFloat32Dataset(file, fieldPath);
        }
        H5Fclose(file);
      }
      std::filesystem::remove(getPath(filename));
    }
    barrier();
    return grid;
  }

  /**
   * The storage sizes of the points and of the field v and the values of v in double precision, as
   * the global master read them back. The other ranks keep the empty defaults.
   */
  struct ReadBackPrecision final {
    bool                opened{false};
    std::size_t         pointBytes{0};
    std::size_t         fieldBytes{0};
    std::vector<double> fieldValues{};
  };

  /**
   * Reads the storage sizes and the values of the field v on the global master and deletes the file.
   */
  ReadBackPrecision readPrecisionAndRemoveOnGlobalMaster(const std::string& filename) {
    ReadBackPrecision precision;
    if (isMaster()) {
      const hid_t file = H5Fopen(getPath(filename).c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      precision.opened = file >= 0;
      if (precision.opened) {
        precision.pointBytes = getStorageBytes(file, "/VTKHDF/Points");
        precision.fieldBytes = getStorageBytes(file, "/VTKHDF/CellData/v");

        const hid_t dataset       = H5Dopen(file, "/VTKHDF/CellData/v", H5P_DEFAULT);
        const hid_t dataSpace     = H5Dget_space(dataset);
        hsize_t     dimensions[2] = {0, 0};
        H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
        precision.fieldValues.resize(dimensions[0]);
        H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, precision.fieldValues.data());
        H5Sclose(dataSpace);
        H5Dclose(dataset);
        H5Fclose(file);
      }
      std::filesystem::remove(getPath(filename));
    }
    return precision;
  }

  void writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::VTKHDFFileWriter writer;
    writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
  }

  void validateSingleQuadrangle(const ReadBackGrid& grid) {
    const int ranks = getNumberOfRanks();
    VALIDATE(grid.opened);
    VALIDATE_EQUALS(grid.numberOfSteps, 1);
    // A reader rejects a file whose version lies above the version it supports.
    VALIDATE(grid.version.size() == 2 and grid.version[0] == 2 and grid.version[1] == 0);
    VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 4 * ranks, grid.numberOfPoints.size());
    VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == ranks, grid.numberOfCells.size());
    VALIDATE(grid.numberOfConnectivityIds.size() == 1 and grid.numberOfConnectivityIds[0] == 4 * ranks);
    VALIDATE_EQUALS(static_cast<int>(grid.types.size()), ranks);
    for (const int type : grid.types) {
      VALIDATE_EQUALS(type, QuadrangleType);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.offsets.size()), ranks + 1);
    for (int cell = 0; cell <= ranks; cell++) {
      VALIDATE_EQUALS(grid.offsets[cell], 4 * cell, cell);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.connectivity.size()), 4 * ranks);
    for (int entry = 0; entry < 4 * ranks; entry++) {
      VALIDATE_EQUALS(grid.connectivity[entry], entry, entry);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.fieldValues.size()), ranks);
    for (int rank = 0; rank < ranks; rank++) {
      VALIDATE_EQUALS(grid.fieldValues[rank], static_cast<float>(rank), rank);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.points.size()), 4 * ranks * 3);
    for (int rank = 0; rank < ranks; rank++) {
      VALIDATE_EQUALS(grid.points[(4 * rank) * 3 + 0], static_cast<float>(2 * rank), rank);
      VALIDATE_EQUALS(grid.points[(4 * rank) * 3 + 2], 0.0F, rank);
    }
  }

  void validateTwoSteps(const ReadBackGrid& grid) {
    const int ranks = getNumberOfRanks();
    VALIDATE(grid.opened);
    VALIDATE_EQUALS(grid.numberOfSteps, 2);
    VALIDATE(grid.stepValues.size() == 2 and grid.stepValues[0] == 0.0F and grid.stepValues[1] == 0.5F);
    VALIDATE(grid.pointOffsets.size() == 2 and grid.pointOffsets[0] == 0 and grid.pointOffsets[1] == 4 * ranks);
    VALIDATE(grid.cellOffsets.size() == 2 and grid.cellOffsets[0] == 0 and grid.cellOffsets[1] == ranks);
    VALIDATE(
      grid.connectivityOffsets.size() == 2 and grid.connectivityOffsets[0] == 0
      and grid.connectivityOffsets[1] == 4 * ranks
    );
    VALIDATE(grid.partOffsets.size() == 2 and grid.partOffsets[0] == 0 and grid.partOffsets[1] == 1);
    VALIDATE(
      grid.numberOfPoints.size() == 2 and grid.numberOfPoints[0] == 4 * ranks and grid.numberOfPoints[1] == 4 * ranks
    );
  }

  void validateMixedCellTypes(const ReadBackGrid& grid) {
    const int ranks = getNumberOfRanks();
    VALIDATE(grid.opened);
    VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 6 * ranks);
    VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == 2 * ranks);
    VALIDATE(grid.numberOfConnectivityIds.size() == 1 and grid.numberOfConnectivityIds[0] == 6 * ranks);
    VALIDATE_EQUALS(static_cast<int>(grid.types.size()), 2 * ranks);
    VALIDATE_EQUALS(static_cast<int>(grid.offsets.size()), 2 * ranks + 1);
    VALIDATE_EQUALS(static_cast<int>(grid.connectivity.size()), 6 * ranks);
    for (int rank = 0; rank < ranks; rank++) {
      VALIDATE_EQUALS(grid.types[2 * rank], LineType, rank);
      VALIDATE_EQUALS(grid.types[2 * rank + 1], QuadrangleType, rank);
      VALIDATE_EQUALS(grid.offsets[2 * rank], 6 * rank, rank);
      VALIDATE_EQUALS(grid.offsets[2 * rank + 1], 6 * rank + 2, rank);
      // The line refers to the points 6r and 6r+1 and the quadrangle to the points 6r+2 to 6r+5.
      VALIDATE_EQUALS(grid.connectivity[6 * rank + 0], 6 * rank + 0, rank);
      VALIDATE_EQUALS(grid.connectivity[6 * rank + 2], 6 * rank + 2, rank);
      VALIDATE_EQUALS(grid.connectivity[6 * rank + 5], 6 * rank + 5, rank);
    }
    VALIDATE_EQUALS(grid.offsets[2 * ranks], 6 * ranks);
  }

  void validateParticles(const ReadBackGrid& grid) {
    const int ranks = getNumberOfRanks();
    VALIDATE(grid.opened);
    VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 2 * ranks);
    VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == 0);
    VALIDATE(grid.offsets.size() == 1 and grid.offsets[0] == 0);
    VALIDATE_EQUALS(static_cast<int>(grid.fieldValues.size()), 2 * ranks);
  }

  void validateHexahedra(const ReadBackGrid& grid) {
    const int ranks = getNumberOfRanks();
    VALIDATE(grid.opened);
    VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == ranks);
    VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 8 * ranks);
    VALIDATE_EQUALS(static_cast<int>(grid.types.size()), ranks);
    for (const int type : grid.types) {
      VALIDATE_EQUALS(type, HexahedronType);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.fieldValues.size()), ranks);
    for (const float value : grid.fieldValues) {
      VALIDATE_EQUALS(value, 2.5F);
    }
    VALIDATE_EQUALS(static_cast<int>(grid.points.size()), 8 * ranks * 3);
    // The top face of every cube lies at z = 1.
    bool anyNonZeroZ = false;
    for (std::size_t point = 0; point < grid.points.size() / 3; point++) {
      anyNonZeroZ = anyNonZeroZ or grid.points[3 * point + 2] != 0.0F;
    }
    VALIDATE(anyNonZeroZ);
  }

  void validateDoublePrecisionFile(const ReadBackPrecision& doubleFile, const double fineValue) {
    VALIDATE(doubleFile.opened);
    VALIDATE_EQUALS(doubleFile.pointBytes, 8);
    VALIDATE_EQUALS(doubleFile.fieldBytes, 8);
    for (const double value : doubleFile.fieldValues) {
      VALIDATE_EQUALS(value, fineValue);
    }
  }
} // namespace

UNIT_TEST("One unit square per rank in one step writes the step totals, the offsets, the points and the cell values") {
  GIVEN("A unit square per rank, shifted in x by twice the rank, with the rank as the cell field v") {
    const std::string    filename = "VTKHDFTestSingleQuadrangle";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 2.0 * getRank(), 1);
    mesh.cellFields.push_back({"v", 1, {static_cast<double>(getRank())}});

    WHEN("The writer writes the mesh and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "/VTKHDF/CellData/v");

      THEN("The one part of the step lists the squares of every rank in rank order with their points and values") {
        if (isMaster()) {
          validateSingleQuadrangle(grid);
        }
      }
    }
  }
}

UNIT_TEST("Every write appends a step whose offsets in Steps count the entries of the steps before") {
  GIVEN("A unit square per rank, shifted in x by twice the rank") {
    const std::string    filename = "VTKHDFTestTwoSteps";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 2.0 * getRank(), 1);

    WHEN("One writer writes the mesh at the times 0 and 0.5 and the global master reads the grid back") {
      tarch::plotter::VTKHDFFileWriter writer;
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.0);
      writer.write(mesh, tarch::testing::getWritableTestDirectory().string(), filename, 0.5);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "");

      THEN("The file contains two steps whose offsets count the points, cells, corners and parts of the step before") {
        if (isMaster()) {
          validateTwoSteps(grid);
        }
      }
    }
  }
}

UNIT_TEST("The file stores a line and a unit square of every rank in one part of the step") {
  GIVEN("A line and a unit square per rank, apart from the cells of the other ranks") {
    const std::string    filename = "VTKHDFTestMixedCellTypes";
    const double         x        = 10.0 * getRank();
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ    = {x, 0.0, 0.0, x + 1.0, 0.0, 0.0};
    mesh.connectivity = {0, 1};
    mesh.offsets      = {2};
    mesh.types        = {static_cast<std::uint8_t>(tarch::plotter::CellType::Line)};
    tarch::plotter::unittests::appendUnitSquares(mesh, x + 2.0, 1);

    WHEN("The writer writes the mesh and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "");

      THEN("Every rank adds a line and a square whose corners follow the corners of the ranks before") {
        if (isMaster()) {
          validateMixedCellTypes(grid);
        }
      }
    }
  }
}

UNIT_TEST("The file of points without cells lists zero cells and the single offset 0") {
  GIVEN("Two points with the point field Mass per rank") {
    const std::string    filename = "VTKHDFTestParticles";
    tarch::plotter::Mesh mesh;
    mesh.pointsXYZ = {0.5 + getRank(), 0.5, 0.0, 0.6 + getRank(), 0.6, 0.0};
    mesh.pointFields.push_back({"Mass", 1, {10.0, 20.0}});

    WHEN("The writer writes the mesh and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "/VTKHDF/PointData/Mass");

      THEN("The grid contains two points per rank, zero cells, the offset 0 and one Mass per point") {
        if (isMaster()) {
          validateParticles(grid);
        }
      }
    }
  }
}

UNIT_TEST("The file stores the points, the type and the cell value of one hexahedron per rank") {
  GIVEN("A unit cube per rank, shifted in x by twice the rank, with the cell field Density 2.5") {
    const std::string    filename = "VTKHDFTest3DHexahedron";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitCube(mesh, 2.0 * getRank());
    mesh.cellFields.push_back({"Density", 1, {2.5}});

    WHEN("The writer writes the mesh and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "/VTKHDF/CellData/Density");

      THEN("The grid contains one hexahedron of eight 3D points per rank with the Density 2.5") {
        if (isMaster()) {
          validateHexahedra(grid);
        }
      }
    }
  }
}

UNIT_TEST("A collective write with cells on the global master alone writes the cells of the global master") {
  GIVEN("A unit square with the cell field Marker 7 on the global master and the empty field Marker elsewhere") {
    const std::string    filename = "VTKHDFTestHeterogeneous";
    tarch::plotter::Mesh mesh;
    if (isMaster()) {
      tarch::plotter::unittests::appendUnitSquares(mesh, 0.0, 1);
      mesh.cellFields.push_back({"Marker", 1, {7.0}});
    } else {
      mesh.cellFields.push_back({"Marker", 1, {}});
    }

    WHEN("Every rank enters the collective write and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "/VTKHDF/CellData/Marker");

      THEN("The grid contains the four points, the cell and the Marker 7 of the global master") {
        if (isMaster()) {
          VALIDATE(grid.opened);
          VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 4);
          VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == 1);
          VALIDATE(grid.fieldValues.size() == 1 and grid.fieldValues[0] == 7.0F);
        }
      }
    }
  }
}

UNIT_TEST("The collective write completes for three cells on the global master and two on every other rank") {
  GIVEN("Three unit squares on the global master and two on every other rank, apart from each other") {
    const std::string    filename = "VTKHDFTestUneven";
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 100.0 * getRank(), isMaster() ? 3 : 2);

    WHEN("The writer writes the mesh and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid                  = readAndRemoveOnGlobalMaster(filename, "");
      const long long    expectedNumberOfCells = 3 + 2 * (getNumberOfRanks() - 1);

      THEN("The one part of the step contains the cells of every rank with four points each") {
        if (isMaster()) {
          VALIDATE(grid.opened);
          VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == expectedNumberOfCells);
          VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 4 * expectedNumberOfCells);
        }
      }
    }
  }
}

UNIT_TEST("A mesh without points, cells and fields writes a valid file with an empty grid") {
  GIVEN("An empty mesh on every rank") {
    const std::string          filename = "VTKHDFTestEmpty";
    const tarch::plotter::Mesh mesh;

    WHEN("Every rank enters the collective write and the global master reads the grid back") {
      writeMesh(mesh, filename);
      const ReadBackGrid grid = readAndRemoveOnGlobalMaster(filename, "");

      THEN("The file contains one step of zero points, zero cells and the single offset 0") {
        if (isMaster()) {
          VALIDATE(grid.opened);
          VALIDATE_EQUALS(grid.numberOfSteps, 1);
          VALIDATE(grid.numberOfPoints.size() == 1 and grid.numberOfPoints[0] == 0);
          VALIDATE(grid.numberOfCells.size() == 1 and grid.numberOfCells[0] == 0);
          VALIDATE(grid.offsets.size() == 1 and grid.offsets[0] == 0);
        }
      }
    }
  }
}

UNIT_TEST("setDoublePrecisionOutput() selects 4-byte or 8-byte reals for the points and the fields") {
  GIVEN("A unit square per rank with a cell value below the resolution of single precision away from one") {
    const double         fineValue = 1.0 + 1.0e-12;
    tarch::plotter::Mesh mesh;
    tarch::plotter::unittests::appendUnitSquares(mesh, 2.0 * getRank(), 1);
    mesh.cellFields.push_back({"v", 1, {fineValue}});

    WHEN("The writer writes the mesh with the default precision and with double precision") {
      const bool writesDoublePrecisionByDefault = tarch::plotter::FileWriter::getDoublePrecisionOutput();
      writeMesh(mesh, "VTKHDFTestSinglePrecision");
      tarch::plotter::FileWriter::setDoublePrecisionOutput(true);
      writeMesh(mesh, "VTKHDFTestDoublePrecision");
      tarch::plotter::FileWriter::setDoublePrecisionOutput(false);
      barrier();
      const ReadBackPrecision singleFile = readPrecisionAndRemoveOnGlobalMaster("VTKHDFTestSinglePrecision");
      const ReadBackPrecision doubleFile = readPrecisionAndRemoveOnGlobalMaster("VTKHDFTestDoublePrecision");
      barrier();

      THEN("The default stores 4-byte reals and double precision stores 8-byte reals that keep the value") {
        VALIDATE(not writesDoublePrecisionByDefault);
        if (isMaster()) {
          VALIDATE(singleFile.opened);
          VALIDATE_EQUALS(singleFile.pointBytes, 4);
          VALIDATE_EQUALS(singleFile.fieldBytes, 4);
          validateDoublePrecisionFile(doubleFile, fineValue);
        }
      }
    }
  }
}

#endif
