// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  /**
   * Throws when the .xmf file does not exist or does not contain the substring.
   */
  void validateXMFContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not std::filesystem::exists(filepath)) {
      throw std::runtime_error("Test failed: Output XMF file was not created: " + filepath);
    }

    std::ifstream     stream(filepath);
    std::stringstream buffer;
    buffer << stream.rdbuf();
    std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error(
        "Test failed: Output XMF file '" + filepath + "' does not contain expected string: '" + expectedSubstring + "'"
      );
    }
  }

  void cleanup(const std::string& filename) {
    std::filesystem::path filePath(filename);
    if (std::filesystem::exists(filePath)) {
      std::filesystem::remove(filePath);
    }

    if (filePath.extension() == ".xmf") {
      std::filesystem::path hdf5Path = filePath;
      hdf5Path.replace_extension(".h5");
      if (std::filesystem::exists(hdf5Path)) {
        std::filesystem::remove(hdf5Path);
      }
    }
  }
} // namespace

tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void
tarch::plotter::tests::HDF5FileWriterTest::testHeterogeneousRanks() {
  // Only the global master plots data, and every rank calls writeToFile().
  const std::string filename = "HDF5TestHeterogeneous";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    auto vertexPlotter   = plotter.createVertexPlotter(0);
    auto cellPlotter     = plotter.createCellPlotter(0);
    auto cellDataPlotter = plotter.createCellDataPlotter(0, "Marker", 1);

    int p0      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int p1      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int p2      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    int p3      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int quad[4] = {p0, p1, p2, p3};
    int cell    = cellPlotter->plotQuadrangle(quad);
    cellDataPlotter->plotCellData(cell, 1.0);
  }

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The "Marker" field dataset must exist even though only one rank held data.
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"Marker\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".h5")).string());
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testInvalidIdentifierThrows() {
  // A slash would corrupt the HDF5 dataset path.
  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
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
tarch::plotter::tests::HDF5FileWriterTest::run() {
  // The read-back helpers report a malformed file by throwing, and the test framework
  // does not guard run(), so this catch records the failure and keeps the binary alive.
  try {
    testMethod(test2DSimpleTriangle);
    testMethod(test2DParticlesWithData);
    testMethod(test3DCellData);
    testMethod(testAttributeTypes);
    testMethod(testVectorAndTensorPadding);
    testMethod(testMultipleSubdomains);
    testMethod(testHeterogeneousRanks);
    testMethod(testUnevenSubdomainCounts);
    testMethod(testEmptyPlotter);
    testMethod(testInconsistentDataSizeThrows);
    testMethod(testInvalidIdentifierThrows);
  } catch (const std::runtime_error& exception) {
    _errors++;
    _error = true;
    std::cerr << "  " << exception.what() << std::endl;
  }
}

void
tarch::plotter::tests::HDF5FileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "HDF5TestUneven";

  // The master plots three quads and every other rank two.
  const int      numberOfSubdomains = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  for (int subdomain = 0; subdomain < numberOfSubdomains; ++subdomain) {
    plotter.acquirePiece(subdomain);
  }
  for (int subdomain = 0; subdomain < numberOfSubdomains; ++subdomain) {
    auto         vertexPlotter = plotter.createVertexPlotter(subdomain);
    auto         cellPlotter   = plotter.createCellPlotter(subdomain);
    const double origin        = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * subdomain;
    int          p0            = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 0.0, 0.0});
    int          p1            = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 1.0, 0.0});
    int          p2            = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 1.0, 1.0});
    int          p3            = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{origin + 0.0, 1.0});
    int          quad[4]       = {p0, p1, p2, p3};
    cellPlotter->plotQuadrangle(quad);
  }

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int cells = 3 + 2 * (ranks - 1);
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "NumberOfElements=\"" + std::to_string(cells) + "\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testVectorAndTensorPadding() {
  const std::string filename = "HDF5TestVectorTensor";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter = plotter.createVertexPlotter(0);
  auto cellPlotter   = plotter.createCellPlotter(0);
  auto vectorPlotter = plotter.createCellDataPlotter(0, "V2", 2);
  auto tensorPlotter = plotter.createCellDataPlotter(0, "T9", 9);

  int p0      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int p1      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
  int p2      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
  int p3      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
  int quad[4] = {p0, p1, p2, p3};
  int cell    = cellPlotter->plotQuadrangle(quad);
  vectorPlotter->plotCellData(cell, tarch::la::Vector<2, double>{1.5, -1.5});
  double tensorValues[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  tensorPlotter->plotCellData(cell, tensorValues, 9);

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The 2D vector is written as a 3-component Vector, the 9-tuple as a Tensor.
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"V2\" AttributeType=\"Vector\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"T9\" AttributeType=\"Tensor\""
    );

    // The vector dataset must be N x 3 with the padded z component zero.
    hid_t file = H5Fopen(
      (tarch::tests::getWritableTestDirectory() / (filename + ".h5")).string().c_str(),
      H5F_ACC_RDONLY,
      H5P_DEFAULT
    );
    hid_t   dataset       = H5Dopen(file, "/cell_data/V2", H5P_DEFAULT);
    hid_t   dataSpace     = H5Dget_space(dataset);
    hsize_t dimensions[2] = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    std::vector<float> values(dimensions[0] * dimensions[1]);
    H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(dataSpace);
    H5Dclose(dataset);
    H5Fclose(file);

    if (dimensions[1] != 3) {
      throw std::runtime_error("Expected the 2D vector field to be written with 3 components.");
    }
    if (values[0] != 1.5F or values[1] != -1.5F or values[2] != 0.0F) {
      throw std::runtime_error("Expected the 2D vector to be padded with a zero z component (1.5, -1.5, 0).");
    }
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "HDF5Test2DTriangle";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter = plotter.createVertexPlotter(0);
  auto cellPlotter   = plotter.createCellPlotter(0);

  int p0 = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int p1 = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
  int p2 = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});

  int indices[] = {p0, p1, p2};
  cellPlotter->plotTriangle(indices);

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "TopologyType=\"Mixed\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "GeometryType=\"X_Y_Z\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      filename + ".h5:/geometry/x"
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::test2DParticlesWithData() {
  const std::string filename = "HDF5Test2DParticles";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter     = plotter.createVertexPlotter(0);
  auto vertexDataPlotter = plotter.createVertexDataPlotter(0, "Mass", 1);

  int p0 = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.5, 0.5});
  vertexDataPlotter->plotVertexData(p0, 10.0);

  int p1 = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.6, 0.6});
  vertexDataPlotter->plotVertexData(p1, 20.0);

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"Mass\" AttributeType=\"Scalar\""
    );
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Center=\"Node\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::test3DCellData() {
  const std::string filename = "HDF5Test3DCellData";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter   = plotter.createVertexPlotter(0);
  auto cellPlotter     = plotter.createCellPlotter(0);
  auto cellDataPlotter = plotter.createCellDataPlotter(0, "Density", 1);

  int cornerIndices[8] = {};
  for (int i = 0; i < 8; i++) {
    cornerIndices[i] = vertexPlotter->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 0.0});
  }

  int cellIndex = cellPlotter->plotHexahedron(cornerIndices);
  cellDataPlotter->plotCellData(cellIndex, 1.234);

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"Density\" AttributeType=\"Scalar\""
    );
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Center=\"Cell\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testAttributeTypes() {
  const std::string filename = "HDF5TestTypes";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  auto vertexPlotter = plotter.createVertexPlotter(0);
  int  p0            = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});

  auto scalarPlotter = plotter.createVertexDataPlotter(0, "ScalarField", 1);
  scalarPlotter->plotVertexData(p0, 1.0);

  auto vectorPlotter = plotter.createVertexDataPlotter(0, "VectorField", 3);
  vectorPlotter->plotVertexData(p0, tarch::la::Vector<3, double>{1.0, 2.0, 3.0});

  // A 4-component field is neither a vector nor a tensor, so it stays generic.
  double matrixValues[4] = {1, 0, 0, 1};
  auto   matrixPlotter   = plotter.createVertexDataPlotter(0, "TensorField", 4);
  matrixPlotter->plotVertexData(p0, matrixValues, 4);

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Name=\"ScalarField\" AttributeType=\"Scalar\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Name=\"VectorField\" AttributeType=\"Vector\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Name=\"TensorField\" AttributeType=\"Matrix\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testMultipleSubdomains() {
  const std::string filename = "HDF5TestMultipleSubdomains";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  plotter.acquirePiece(1);

  {
    auto vertexPlotter   = plotter.createVertexPlotter(0);
    auto cellPlotter     = plotter.createCellPlotter(0);
    auto cellDataPlotter = plotter.createCellDataPlotter(0, "Marker", 1);

    int p0          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int p1          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int p2          = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int triangle[3] = {p0, p1, p2};
    int cell        = cellPlotter->plotTriangle(triangle);
    cellDataPlotter->plotCellData(cell, 1.0);
  }

  {
    auto vertexPlotter   = plotter.createVertexPlotter(1);
    auto cellPlotter     = plotter.createCellPlotter(1);
    auto cellDataPlotter = plotter.createCellDataPlotter(1, "Marker", 1);

    int p0      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
    int p1      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
    int p2      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{3.0, 1.0});
    int p3      = vertexPlotter->plotVertex(tarch::la::Vector<2, double>{2.0, 1.0});
    int quad[4] = {p0, p1, p2, p3};
    int cell    = cellPlotter->plotQuadrangle(quad);
    cellDataPlotter->plotCellData(cell, 2.0);
  }

  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "NumberOfElements=\"" + std::to_string(2 * ranks) + "\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Dimensions=\"" + std::to_string(7 * ranks) + "\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Dimensions=\"" + std::to_string(9 * ranks) + "\""
    );
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "Attribute Name=\"Marker\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testEmptyPlotter() {
  const std::string filename = "HDF5TestEmpty";
  HDF5FileWriter    plotter(tarch::tests::getWritableTestDirectory().string());
  plotter.acquirePiece(0);
  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "NumberOfElements=\"0\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void
tarch::plotter::tests::HDF5FileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "HDF5TestInconsistent";

  bool exceptionThrown = false;
  try {
    HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string());
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
  }

  if (not exceptionThrown) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
    }
    throw std::runtime_error("HDF5FileWriter did not throw on inconsistent data size.");
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

#else // No HDF5

// A build without HDF5 runs an empty test.
tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void
tarch::plotter::tests::HDF5FileWriterTest::run() {}

#endif
