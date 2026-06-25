// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriterTest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/NetCDFFileWriter.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

#ifdef PEANO_WITH_NETCDF

namespace {
  void validateFileExists(const std::string& filepath) {
    if (not fs::exists(filepath)) {
      throw std::runtime_error("Test failed: Output NetCDF file was not created: " + filepath);
    }
  }

  void cleanup(const std::string& filename) {
    fs::path p(filename);
    if (fs::exists(p)) {
      fs::remove(p);
    }
  }
} // namespace

tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest():
  TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void tarch::plotter::tests::NetCDFFileWriterTest::run() {
  testMethod(test2DSimpleTriangle);
  testMethod(test2DQuadWithData);
  testMethod(testEmptyPlotter);
  testMethod(testInconsistentDataSizeThrows);
  testMethod(testUnsupportedCellType);
}

void tarch::plotter::tests::NetCDFFileWriterTest::test2DSimpleTriangle() {
  const std::string filename = "NetCDFTestTriangle";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v = plotter.createVertexPlotter(0);
    auto             c = plotter.createCellPlotter(0);

    int i0         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int i1         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int i2         = v->plotVertex(tarch::la::Vector<2, double>{0.5, 1.0});
    int indices[3] = {i0, i1, i2};

    c->plotTriangle(indices);

    plotter.writeToFile(filename, 0);
  }

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::test2DQuadWithData() {
  const std::string filename = "NetCDFTestQuad";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             c  = plotter.createCellPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "Pressure", 1);
    auto             cd = plotter.createCellDataPlotter(0, "Velocity", 2);

    int i0         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    int i1         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 0.0});
    int i2         = v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});
    int i3         = v->plotVertex(tarch::la::Vector<2, double>{0.0, 1.0});
    int indices[4] = {i0, i1, i2, i3};

    int cellIndex = c->plotQuadrangle(indices);

    // Add Data
    vd->plotVertexData(i0, 10.0);
    vd->plotVertexData(i1, 11.0);
    vd->plotVertexData(i2, 12.0);
    vd->plotVertexData(i3, 13.0);

    cd->plotCellData(cellIndex, tarch::la::Vector<2, double>{1.5, -1.5});

    plotter.writeToFile(filename, 0);
  }

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    validateFileExists(fullPath);
    cleanup(fullPath);
  }
  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::testEmptyPlotter() {
  const std::string filename = "NetCDFTestEmpty";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    // Write without adding any data
    plotter.writeToFile(filename, 0);
  }

  tarch::mpi::Rank::getInstance().barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    if (fs::exists(fullPath)) {
      cleanup(fullPath);
    }
  }
  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::testInconsistentDataSizeThrows() {
  const std::string filename = "NetCDFTestInconsistent";
  const std::string fullPath = (tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string();

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v  = plotter.createVertexPlotter(0);
    auto             vd = plotter.createVertexDataPlotter(0, "BrokenField", 1);

    v->plotVertex(tarch::la::Vector<2, double>{0.0, 0.0});
    v->plotVertex(tarch::la::Vector<2, double>{1.0, 1.0});

    // Plot data for only 1 vertex (Bug!)
    vd->plotVertexData(0, 5.0);

    // This should throw/fail during write or merge
    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  } catch (...) {
    // Catch other exceptions too
    exceptionThrown = true;
  }

  tarch::mpi::Rank::getInstance().barrier();
  if (not exceptionThrown) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup(fullPath);
    }
    throw std::runtime_error("NetCDFFileWriter did not throw on inconsistent data size!");
  } else {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup(fullPath);
    }
  }
  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::tests::NetCDFFileWriterTest::testUnsupportedCellType() {
  const std::string filename = "NetCDFTestUnsupported";

  bool exceptionThrown = false;
  try {
    NetCDFFileWriter plotter(tarch::tests::getWritableTestDirectory().string(), 1);
    auto             v = plotter.createVertexPlotter(0);
    auto             c = plotter.createCellPlotter(0);

    int indices[8];

    // Bottom face (z=0)
    indices[0] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 0.0});
    indices[1] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 0.0});
    indices[2] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 0.0});
    indices[3] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 0.0});

    // Top face (z=1)
    indices[4] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 0.0, 1.0});
    indices[5] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 0.0, 1.0});
    indices[6] = v->plotVertex(tarch::la::Vector<3, double>{1.0, 1.0, 1.0});
    indices[7] = v->plotVertex(tarch::la::Vector<3, double>{0.0, 1.0, 1.0});

    c->plotHexahedron(indices);

    plotter.writeToFile(filename, 0);
  } catch (const std::runtime_error&) {
    exceptionThrown = true;
  }

  tarch::mpi::Rank::getInstance().barrier();
  if (not exceptionThrown) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      cleanup((tarch::tests::getWritableTestDirectory() / (filename + ".nc")).string());
    }
    throw std::runtime_error("NetCDFFileWriter did not throw on unsupported cell type!");
  }
  tarch::mpi::Rank::getInstance().barrier();
}

#else // PEANO_WITH_NETCDF

// Dummy implementation for compilation when NetCDF is disabled
tarch::plotter::tests::NetCDFFileWriterTest::NetCDFFileWriterTest():
  TestCase("tarch::plotter::tests::NetCDFFileWriterTest") {}

void tarch::plotter::tests::NetCDFFileWriterTest::run() {
  // Do nothing
}

#endif
