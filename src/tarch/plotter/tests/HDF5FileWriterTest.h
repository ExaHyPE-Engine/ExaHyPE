// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class HDF5FileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::HDF5FileWriterTest final : public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test2DParticlesWithData();
  void test3DCellData();
  void testAttributeTypes();
  void testMultipleSubdomains();

  /**
   * A 2-component field is a 2D vector: it must be written with 3 components (the
   * z padded to zero) and labelled AttributeType="Vector", and a 9-component
   * field as AttributeType="Tensor", so the field reads back as a real vector /
   * tensor. See FileWriter::classifyField.
   */
  void testVectorAndTensorPadding();

  /**
   * Regression test for the collective field-schema deadlock: only the global
   * master contributes data while every other rank stays empty. The collective
   * parallel write must not deadlock, which relies on the field schema being
   * reduced globally so empty ranks join every dataset call.
   */
  void testHeterogeneousRanks();

  /**
   * An untouched plotter must produce a valid empty file.
   */
  void testEmptyPlotter();

  /**
   * The plotter must throw if data fields do not match the geometry size.
   */
  void testInconsistentDataSizeThrows();

  /**
   * Checks that a data identifier containing a character that would corrupt the
   * XML / HDF5 path / NetCDF name is rejected.
   */
  void testInvalidIdentifierThrows();

  /**
   * Ranks hold different non-zero subdomain counts (master 3, the rest 2) - the
   * real load-balanced distribution that the uniform/master-vs-empty MPI tests
   * never exercised. The collective parallel write must not deadlock and the
   * merged grid must carry the summed cell total.
   */
  void testUnevenSubdomainCounts();

public:
  HDF5FileWriterTest();
  ~HDF5FileWriterTest() override = default;

  void run() override;
};
