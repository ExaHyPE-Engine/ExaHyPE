// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTKHDFFileWriterTest;
} // namespace tarch::plotter::tests

/**
 * Checks the VTKHDF layout of the written files through the HDF5 C API.
 */
class tarch::plotter::tests::VTKHDFFileWriterTest final : public tarch::tests::TestCase {
public:
  VTKHDFFileWriterTest();
  VTKHDFFileWriterTest(const VTKHDFFileWriterTest& other)            = delete;
  VTKHDFFileWriterTest(VTKHDFFileWriterTest&& other)                 = delete;
  VTKHDFFileWriterTest& operator=(const VTKHDFFileWriterTest& other) = delete;
  VTKHDFFileWriterTest& operator=(VTKHDFFileWriterTest&& other)      = delete;
  ~VTKHDFFileWriterTest() override                                   = default;

  void run() override;

private:
  void testSingleQuadrangle2D();
  void testTwoTimeSteps();
  void testMixedCellTypes();
  void testParticlePointCloud();
  void test3DHexahedron();
  void testHeterogeneousRanks();
  void testUnevenCellCounts();
  void testEmptyMesh();
  void testPrecisionSwitch();
};
