// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriterTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/tests/Meshes.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  std::string getPath(const std::string& name) { return (tarch::tests::getWritableTestDirectory() / name).string(); }

  /**
   * Throws when the file does not exist or does not contain the substring.
   */
  void validateXMFContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("The test expects the file '" + filepath + "'.");
    }

    const std::ifstream stream(filepath);
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error("The test expects '" + expectedSubstring + "' in the file '" + filepath + "'.");
    }
  }

  /**
   * Writes mesh as the snapshot at the time 0 and returns the path of its .xmf file.
   */
  std::string writeMesh(const tarch::plotter::Mesh& mesh, const std::string& filename) {
    tarch::plotter::HDF5FileWriter writer;
    writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
    barrier();
    return getPath(filename + "-0_000000.xmf");
  }

  void removeSeries(const std::string& filename) {
    barrier();
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      for (const std::string& name : {filename + "-0_000000.xmf", filename + "-0_000000.h5", filename + ".xmf"}) {
        std::filesystem::remove(getPath(name));
      }
    }
    barrier();
  }
} // namespace

tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void
tarch::plotter::tests::HDF5FileWriterTest::run() {
  // The read-back helpers throw for a malformed file. An exception that leaves run() ends the test
  // executable.
  try {
    testMethod(test2DSimpleQuadrangle);
    testMethod(test2DParticlesWithData);
    testMethod(test3DCellData);
    testMethod(testAttributeTypes);
    testMethod(testVectorAndTensorPadding);
    testMethod(testMixedCellTypes);
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

/**
 * The collective write completes when only the global master writes cells.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testHeterogeneousRanks() {
  const std::string filename = "HDF5TestHeterogeneous";

  tarch::plotter::Mesh mesh;
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    appendUnitSquares(mesh, 0.0, 1);
    mesh.cellFields.push_back({"Marker", 1, {1.0}});
  } else {
    mesh.cellFields.push_back({"Marker", 1, {}});
  }
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "NumberOfElements=\"1\"");
    validateXMFContent(xmfPath, "Attribute Name=\"Marker\"");
  }
  removeSeries(filename);
}

/**
 * The grid of a collective write over ranks with different cell counts contains the cells of all
 * ranks.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testUnevenCellCounts() {
  const std::string filename = "HDF5TestUneven";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(
    mesh,
    100.0 * tarch::mpi::Rank::getInstance().getRank(),
    tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2
  );
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const int cells = 3 + 2 * (tarch::mpi::Rank::getInstance().getNumberOfRanks() - 1);
    validateXMFContent(xmfPath, "NumberOfElements=\"" + std::to_string(cells) + "\"");
  }
  removeSeries(filename);
}

/**
 * The writer pads a field with two components to a Vector with three components.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testVectorAndTensorPadding() {
  const std::string filename = "HDF5TestVectorTensor";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  mesh.cellFields.push_back({"V2", 2, {1.5, -1.5}});
  mesh.cellFields.push_back({"T9", 9, {1, 2, 3, 4, 5, 6, 7, 8, 9}});
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "Attribute Name=\"V2\" AttributeType=\"Vector\"");
    validateXMFContent(xmfPath, "Attribute Name=\"T9\" AttributeType=\"Tensor\"");

    const hid_t file          = H5Fopen(getPath(filename + "-0_000000.h5").c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t dataset       = H5Dopen(file, "/cell_data/V2", H5P_DEFAULT);
    const hid_t dataSpace     = H5Dget_space(dataset);
    hsize_t     dimensions[2] = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    std::vector<float> values(dimensions[0] * dimensions[1]);
    H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    H5Fclose(file);

    if (dimensions[1] != 3) {
      throw std::runtime_error("The test expects three components per cell in the dataset /cell_data/V2.");
    }
    if (values[0] != 1.5F or values[1] != -1.5F or values[2] != 0.0F) {
      throw std::runtime_error("The test expects the padded vector (1.5, -1.5, 0) in the dataset /cell_data/V2.");
    }
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::HDF5FileWriterTest::test2DSimpleQuadrangle() {
  const std::string filename = "HDF5Test2DQuadrangle";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "TopologyType=\"Mixed\"");
    validateXMFContent(xmfPath, "GeometryType=\"X_Y_Z\"");
    validateXMFContent(xmfPath, filename + "-0_000000.h5:/geometry/x");
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::HDF5FileWriterTest::test2DParticlesWithData() {
  const std::string filename = "HDF5Test2DParticles";

  tarch::plotter::Mesh mesh;
  mesh.pointsXYZ = {0.5, 0.5, 0.0, 0.6, 0.6, 0.0};
  mesh.pointFields.push_back({"Mass", 1, {10.0, 20.0}});
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "Attribute Name=\"Mass\" AttributeType=\"Scalar\"");
    validateXMFContent(xmfPath, "Center=\"Node\"");
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::HDF5FileWriterTest::test3DCellData() {
  const std::string filename = "HDF5Test3DCellData";

  tarch::plotter::Mesh mesh;
  appendUnitCube(mesh, 0.0);
  mesh.cellFields.push_back({"Density", 1, {1.234}});
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "Attribute Name=\"Density\" AttributeType=\"Scalar\"");
    validateXMFContent(xmfPath, "Center=\"Cell\"");
  }
  removeSeries(filename);
}

void
tarch::plotter::tests::HDF5FileWriterTest::testAttributeTypes() {
  const std::string filename = "HDF5TestTypes";

  tarch::plotter::Mesh mesh;
  mesh.pointsXYZ = {0.0, 0.0, 0.0};
  mesh.pointFields.push_back({"ScalarField", 1, {1.0}});
  mesh.pointFields.push_back({"VectorField", 3, {1.0, 2.0, 3.0}});
  // The function classifyField() returns FieldKind::Generic for a field with four components.
  mesh.pointFields.push_back({"TensorField", 4, {1.0, 0.0, 0.0, 1.0}});
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "Name=\"ScalarField\" AttributeType=\"Scalar\"");
    validateXMFContent(xmfPath, "Name=\"VectorField\" AttributeType=\"Vector\"");
    validateXMFContent(xmfPath, "Name=\"TensorField\" AttributeType=\"Matrix\"");
  }
  removeSeries(filename);
}

/**
 * The mixed topology lists the node count of a line and none of a quadrangle.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testMixedCellTypes() {
  const std::string filename = "HDF5TestMixedCellTypes";

  tarch::plotter::Mesh mesh;
  const double         x = 10.0 * tarch::mpi::Rank::getInstance().getRank();
  mesh.pointsXYZ         = {x, 0.0, 0.0, x + 1.0, 0.0, 0.0};
  mesh.connectivity      = {0, 1};
  mesh.offsets           = {2};
  mesh.types             = {static_cast<std::uint8_t>(tarch::plotter::CellType::Line)};
  appendUnitSquares(mesh, x + 2.0, 1);
  mesh.cellFields.push_back({"Marker", 1, {1.0, 2.0}});
  const std::string xmfPath = writeMesh(mesh, filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    validateXMFContent(xmfPath, "NumberOfElements=\"" + std::to_string(2 * ranks) + "\"");
    validateXMFContent(xmfPath, "Dimensions=\"" + std::to_string(6 * ranks) + "\"");
    validateXMFContent(xmfPath, "Dimensions=\"" + std::to_string(9 * ranks) + "\"");
    validateXMFContent(xmfPath, "Attribute Name=\"Marker\"");
  }
  removeSeries(filename);
}

/**
 * A mesh without points, cells and fields gives a valid empty file.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testEmptyMesh() {
  const std::string filename = "HDF5TestEmpty";

  const std::string xmfPath = writeMesh(tarch::plotter::Mesh(), filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(xmfPath, "NumberOfElements=\"0\"");
  }
  removeSeries(filename);
}

/**
 * The .xmf file of the series embeds the grid of every snapshot file that one writer writes once.
 */
void
tarch::plotter::tests::HDF5FileWriterTest::testSeries() {
  const std::string filename = "HDF5TestSeries";

  tarch::plotter::Mesh mesh;
  appendUnitSquares(mesh, 0.0, 1);
  tarch::plotter::HDF5FileWriter writer;
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.0);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.5);
  writer.write(mesh, tarch::tests::getWritableTestDirectory().string(), filename, 0.5);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(getPath(filename + ".xmf"), "<Time Value=\"0\"/>");
    validateXMFContent(getPath(filename + ".xmf"), "<Time Value=\"0.5\"/>");
    validateXMFContent(getPath(filename + ".xmf"), filename + "-0_500000.h5:/topology");
    const std::ifstream stream(getPath(filename + ".xmf"));
    std::stringstream   buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();
    if (content.find("<Time Value=\"0.5\"/>") != content.rfind("<Time Value=\"0.5\"/>")) {
      throw std::runtime_error("The test expects the snapshot at the time 0.5 once in the file HDF5TestSeries.xmf.");
    }
    for (const std::string& name :
         {filename + "-0_000000.xmf",
          filename + "-0_000000.h5",
          filename + "-0_500000.xmf",
          filename + "-0_500000.h5",
          filename + ".xmf"}) {
      std::filesystem::remove(getPath(name));
    }
  }
  barrier();
}

#else // No HDF5

// A build without HDF5 runs no HDF5 test.
tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void
tarch::plotter::tests::HDF5FileWriterTest::run() {}

#endif
