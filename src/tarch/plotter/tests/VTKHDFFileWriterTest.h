// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTKHDFFileWriterTest;
} // namespace tarch::plotter::tests

/**
 * Reads the written .vtkhdf back through the HDF5 C API and checks the VTKHDF
 * UnstructuredGrid layout that a reader needs to slice each step.
 */
class tarch::plotter::tests::VTKHDFFileWriterTest final : public tarch::tests::TestCase {
private:
  /**
   * One quad written in one step.
   */
  void testSingleQuad2D();

  void testVertexMerging();

  /**
   * Two traversals appending to one file.
   */
  void testTwoTimeSteps();

  /**
   * Two subdomains merged into a single part.
   */
  void testMultipleSubdomains();

  /**
   * A pure point cloud without any cells.
   */
  void testParticlePointCloud();

  /**
   * A 3D hexahedron with cell data and non-zero z coordinates.
   */
  void test3DHexahedron();

  /**
   * Only the global master holds data.
   */
  void testHeterogeneousRanks();

  /**
   * Ranks hold different non-zero subdomain counts.
   */
  void testUnevenSubdomainCounts();

  /**
   * An untouched plotter produces a valid file with a zero-size grid.
   */
  void testEmptyPlotter();

  /**
   * Field data that does not match the geometry size must be rejected on write.
   */
  void testInconsistentDataSizeThrows();

  /**
   * A data identifier with a character that would corrupt the HDF5 path is
   * rejected up front.
   */
  void testInvalidIdentifierThrows();

  /**
   * The precision switch selects the storage type.
   */
  void testPrecisionSwitch();

public:
  VTKHDFFileWriterTest();
  ~VTKHDFFileWriterTest() override = default;

  void run() override;
};
