// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5FileWriter.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

#ifdef PEANO_WITH_HDF5

namespace {
  /**
   * Checks if the generated .xmf file contains specific substrings.
   * This is used to validate metadata (AttributeTypes, TopologyType, etc.)
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

    // Also clean up associated .h5 or .xmf files
    if (p.extension() == ".xmf") {
      fs::path h5 = p;
      h5.replace_extension(".h5");
      if (fs::exists(h5)) {
        fs::remove(h5);
      }
    }
  }
} // namespace

tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest():
  TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void tarch::plotter::tests::HDF5FileWriterTest::run() {
  testMethod(test2DSimpleTriangle);
  testMethod(test2DParticlesWithData);
  testMethod(test3DCellData);
  testMethod(testAttributeTypes);
  testMethod(testEmptyPlotter);
  testMethod(testInconsistentDataSizeThrows);
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

  tarch::mpi::Rank::getInstance().barrier();
  // Validate XMF Metadata
  // Topology Type 4 is Triangle in XDMF/VTK
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "TopologyType=\"Mixed\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "GeometryType=\"XYZ\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), filename + ".h5:/geometry");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
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

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Attribute Name=\"Mass\" AttributeType=\"Scalar\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Center=\"Node\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
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

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Attribute Name=\"Density\" AttributeType=\"Scalar\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Center=\"Cell\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
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

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Name=\"ScalarField\" AttributeType=\"Scalar\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Name=\"VectorField\" AttributeType=\"Vector\"");
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "Name=\"TensorField\" AttributeType=\"Matrix\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::tests::HDF5FileWriterTest::testEmptyPlotter() {
  const std::string filename = "HDF5TestEmpty";
  HDF5FileWriter    plotter(tarch::tests::getWritableTestDirectory().string(), 1);
  plotter.writeToFile(filename, 0);

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // Should create a valid empty file
    validateXMFContent((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string(), "NumberOfElements=\"0\"");
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
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

    // This should throw in mergeSubdomains() because field size (1) != geom size (2)
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

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".xmf")).string());
  }
  tarch::mpi::Rank::getInstance().barrier();
}

#else // No HDF5

// Empty dummy implementation to satisfy linker if compiled without HDF5
tarch::plotter::tests::HDF5FileWriterTest::HDF5FileWriterTest():
  TestCase("tarch::plotter::tests::HDF5FileWriterTest") {}

void tarch::plotter::tests::HDF5FileWriterTest::run() {}

#endif
