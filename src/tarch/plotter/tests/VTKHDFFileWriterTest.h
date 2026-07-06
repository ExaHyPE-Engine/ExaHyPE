// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTKHDFFileWriterTest;
} // namespace tarch::plotter::tests

/**
 * Tests for the single-file VTKHDF writer. Unlike the HDF5/XDMF test (which only
 * inspects the .xmf text), these read the produced .vtkhdf back through the HDF5
 * C API and assert the actual VTKHDF UnstructuredGrid layout: one merged grid
 * (part) per step, connectivity rebased to global point indices, and the
 * cumulative /VTKHDF/Steps offsets a reader needs to slice each step.
 */
class tarch::plotter::tests::VTKHDFFileWriterTest final : public tarch::tests::TestCase {
private:
  /**
   * One quad, one step: checks NumberOf* per-step totals, Offsets=[0,4], the
   * vertex coordinates and the cell-data value round-trip.
   */
  void testSingleQuad2D();

  /**
   * Two PlotSolution traversals append to one file: NSteps grows to 2 and the
   * Steps offsets are the cumulative prior-step sizes (the reader slice keys).
   */
  void testTwoTimeSteps();

  /**
   * Two subdomains are merged into a single part for the step (not two), and the
   * second subdomain's connectivity is rebased past the first's points. Scales
   * with the MPI rank count.
   */
  void testMultipleSubdomains();

  /**
   * A pure point cloud (vertices + vertex data, no cells), as produced by the
   * particle plotter: NumberOfCells==0 and Offsets==[0].
   */
  void testParticlePointCloud();

  /**
   * A 3D hexahedron (VTK type 12) with cell data and non-zero z coordinates, to
   * cover the 3D geometry/connectivity path alongside the 2D tests.
   */
  void test3DHexahedron();

  /**
   * Only the global master holds data; every other rank is empty. The collective
   * parallel write must not deadlock (every rank issues exactly one collective
   * write per dataset), and the merged grid carries only the master's cells.
   */
  void testHeterogeneousRanks();

  /**
   * Ranks hold different non-zero subdomain counts (master 3, the rest 2) - the
   * real load-balanced distribution. A per-subdomain collective write loop would
   * mismatch the collective call count across ranks and deadlock; the merge-once
   * writer issues exactly one collective write per dataset, so it stays balanced.
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

public:
  VTKHDFFileWriterTest();
  ~VTKHDFFileWriterTest() override = default;

  void run() override;
};
