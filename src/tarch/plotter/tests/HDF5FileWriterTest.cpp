// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/plotter/HDF5FileWriter.h"

namespace fs = std::filesystem;

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() {
#ifdef PEANO_WITH_MPI
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
  }

  /**
   * Checks that the generated .xmf file contains a substring, used to validate
   * metadata (AttributeType, TopologyType, etc.).
   */
  void validateXMFContent(const std::string& filepath, const std::string& expectedSubstring) {
    if (not fs::exists(filepath)) {
      throw std::runtime_error("Test failed: Output XMF file was not created: " + filepath);
    }

    std::ifstream     f(filepath);
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string content = buffer.str();

    if (content.find(expectedSubstring) == std::string::npos) {
      throw std::runtime_error(
        "Test failed: Output XMF file '" + filepath + "' does not contain expected string: '" + expectedSubstring + "'"
      );
    }
  }

  void cleanup(const std::string& filename) {
    fs::path p(filename);
    if (fs::exists(p)) {
      fs::remove(p);
    }

    // Also remove the associated .h5 file
    if (p.extension() == ".xmf") {
      fs::path h5 = p;
      h5.replace_extension(".h5");
      if (fs::exists(h5)) {
        fs::remove(h5);
      }
    }
  }
} // namespace

tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void tarch::plotter::tests::HDF5FileWriterTest::testHeterogeneousRanks() {
  // Only the global master contributes geometry + a "Marker" cell field; every
  // other rank leaves the plotter empty. With parallel collective IO the empty
  // ranks must still participate in every dataset create/write, which works only
  // because the field schema is reduced globally. Without that, writeToFile
  // deadlocks for more than one rank.
  const std::string filename = "HDF5TestHeterogeneous";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    auto v  = plotter.createVertexPlotter(0);
    auto c  = plotter.createCellPlotter(0);
    auto cd = plotter.createCellDataPlotter(0, "Marker", 1);

    int p0      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int p1      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int p2      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    int p3      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int quad[4] = {p0, p1, p2, p3};
    int cell    = c->plotQuadrangle(quad);
    cd->plotCellData(cell, 1.0);
  }
  // All non-master ranks intentionally contribute nothing.

  plotter.writeToFile(filename, 0); // collective; must not deadlock

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

void tarch::plotter::tests::HDF5FileWriterTest::testInvalidIdentifierThrows() {
  // A data identifier is written into the XML, an HDF5 path and a NetCDF name, so
  // a character that would corrupt those (here '/') must be rejected up front.
  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);

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

void tarch::plotter::tests::HDF5FileWriterTest::run() {
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
}

void tarch::plotter::tests::HDF5FileWriterTest::testUnevenSubdomainCounts() {
  const std::string filename = "HDF5TestUneven";

  // Master contributes 3 quads, every other rank 2 - non-zero but unequal counts.
  const int      nSub = tarch::mpi::Rank::getInstance().isGlobalMaster() ? 3 : 2;
  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), nSub);
  for (int sd = 0; sd < nSub; ++sd) {
    auto         v       = plotter.createVertexPlotter(sd);
    auto         c       = plotter.createCellPlotter(sd);
    const double s       = 100.0 * tarch::mpi::Rank::getInstance().getRank() + 2.0 * sd;
    int          p0      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 0.0});
    int          p1      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 0.0});
    int          p2      = v->plotVertex(tarch::la::Vector<2, double>{s + 1.0, 1.0});
    int          p3      = v->plotVertex(tarch::la::Vector<2, double>{s + 0.0, 1.0});
    int          quad[4] = {p0, p1, p2, p3};
    c->plotQuadrangle(quad);
  }

  plotter.writeToFile(filename, 0); // uneven non-zero counts; must not deadlock

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int cells = 3 + 2 * (ranks - 1); // master 3, the other ranks 2 each
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "NumberOfElements=\"" + std::to_string(cells) + "\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileWriterTest::testVectorAndTensorPadding() {
  const std::string filename = "HDF5TestVectorTensor";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto           v    = plotter.createVertexPlotter(0);
  auto           c    = plotter.createCellPlotter(0);
  auto           vec2 = plotter.createCellDataPlotter(0, "V2", 2); // 2D vector
  auto           ten9 = plotter.createCellDataPlotter(0, "T9", 9); // full tensor

  int p0      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int p1      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
  int p2      = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
  int p3      = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
  int quad[4] = {p0, p1, p2, p3};
  int cell    = c->plotQuadrangle(quad);
  vec2->plotCellData(cell, tarch::la::Vector<2, double>{1.5, -1.5});
  double t9[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  ten9->plotCellData(cell, t9, 9);

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
    hid_t   dataset = H5Dopen(file, "/cell_data/V2", H5P_DEFAULT);
    hid_t   space   = H5Dget_space(dataset);
    hsize_t dims[2] = {0, 0};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    std::vector<float> values(dims[0] * dims[1]);
    H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
    H5Sclose(space);
    H5Dclose(dataset);
    H5Fclose(file);

    if (dims[1] != 3) {
      throw std::runtime_error("Expected the 2D vector field to be written with 3 components.");
    }
    if (values[0] != 1.5f or values[1] != -1.5f or values[2] != 0.0f) {
      throw std::runtime_error("Expected the 2D vector to be padded with a zero z component (1.5, -1.5, 0).");
    }
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "HDF5Test2DTriangle";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto           v = plotter.createVertexPlotter(0);
  auto           c = plotter.createCellPlotter(0);

  // Define a triangle
  int p0 = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
  int p1 = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
  int p2 = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});

  int indices[] = {p0, p1, p2};
  c->plotTriangle(indices);

  plotter.writeToFile(filename, 0);

  barrier();
  // Validate XMF metadata.
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

void tarch::plotter::tests::HDF5FileWriterTest::test2DParticlesWithData() {
  const std::string filename = "HDF5Test2DParticles";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto           v  = plotter.createVertexPlotter(0);
  auto           vd = plotter.createVertexDataPlotter(0, "Mass", 1);

  // Plot 2 particles
  int p0 = v->plotVertex(tarch::la::Vector<2, double>{0.5, 0.5});
  vd->plotVertexData(p0, 10.0);

  int p1 = v->plotVertex(tarch::la::Vector<2, double>{0.6, 0.6});
  vd->plotVertexData(p1, 20.0);

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

void tarch::plotter::tests::HDF5FileWriterTest::test3DCellData() {
  const std::string filename = "HDF5Test3DCellData";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto           v  = plotter.createVertexPlotter(0);
  auto           c  = plotter.createCellPlotter(0);
  auto           cd = plotter.createCellDataPlotter(0, "Density", 1);

  // Simple Tet or Hex
  int idx[8];
  for (int i = 0; i < 8; i++) {
    idx[i] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 0.0});
  }

  int cellIdx = c->plotHexahedron(idx);
  cd->plotCellData(cellIdx, 1.234);

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

void tarch::plotter::tests::HDF5FileWriterTest::testAttributeTypes() {
  const std::string filename = "HDF5TestTypes";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  auto           v  = plotter.createVertexPlotter(0);
  int            p0 = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});

  // Scalar (1 component)
  auto s = plotter.createVertexDataPlotter(0, "ScalarField", 1);
  s->plotVertexData(p0, 1.0);

  // Vector (3 components)
  auto vec = plotter.createVertexDataPlotter(0, "VectorField", 3);
  vec->plotVertexData(p0, tarch::la::Vector<3, double>{1.0, 2.0, 3.0});

  // Matrix/Tensor (4 components)
  double mat4[4] = {1, 0, 0, 1};
  auto   mat     = plotter.createVertexDataPlotter(0, "TensorField", 4);
  mat->plotVertexData(p0, mat4, 4);

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

void tarch::plotter::tests::HDF5FileWriterTest::testMultipleSubdomains() {
  const std::string filename = "HDF5TestMultipleSubdomains";

  HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 2);

  {
    auto v  = plotter.createVertexPlotter(0);
    auto c  = plotter.createCellPlotter(0);
    auto cd = plotter.createCellDataPlotter(0, "Marker", 1);

    int p0          = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int p1          = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int p2          = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int triangle[3] = {p0, p1, p2};
    int cell        = c->plotTriangle(triangle);
    cd->plotCellData(cell, 1.0);
  }

  {
    auto v  = plotter.createVertexPlotter(1);
    auto c  = plotter.createCellPlotter(1);
    auto cd = plotter.createCellDataPlotter(1, "Marker", 1);

    int p0      = v->plotVertex(tarch::la::Vector<2, double>{2.0, 0.0});
    int p1      = v->plotVertex(tarch::la::Vector<2, double>{3.0, 0.0});
    int p2      = v->plotVertex(tarch::la::Vector<2, double>{3.0, 1.0});
    int p3      = v->plotVertex(tarch::la::Vector<2, double>{2.0, 1.0});
    int quad[4] = {p0, p1, p2, p3};
    int cell    = c->plotQuadrangle(quad);
    cd->plotCellData(cell, 2.0);
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

void tarch::plotter::tests::HDF5FileWriterTest::testEmptyPlotter() {
  const std::string filename = "HDF5TestEmpty";
  HDF5FileWriter    plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  plotter.writeToFile(filename, 0);

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // Must produce a valid empty file
    validateXMFContent(
      (tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(),
      "NumberOfElements=\"0\""
    );
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "HDF5TestInconsistent";

  bool exceptionThrown = false;
  try {
    HDF5FileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto           v  = plotter.createVertexPlotter(0);
    auto           vd = plotter.createVertexDataPlotter(0, "BrokenField", 1);

    // Plot 2 vertices
    v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});

    // Plot data for only 1 vertex (Bug!)
    vd->plotVertexData(0, 5.0);

    // This should throw during write validation because field size (1) != geom size (2)
    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  if (not exceptionThrown) {
    // Clean up if it failed to throw
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
    }
    throw std::runtime_error("HDF5FileWriter did not throw on inconsistent data size!");
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  barrier();
}

#else // No HDF5

// Dummy implementation to satisfy the linker when compiled without HDF5
tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest() :
  TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void tarch::plotter::tests::HDF5FileWriterTest::run() {}

#endif
