// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class HDF5FileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::HDF5FileWriterTest final : public tarch::tests::TestCase {
public:
  HDF5FileWriterTest();
  ~HDF5FileWriterTest() override = default;

  void run() override;

private:
  void test2DSimpleTriangle();
  void test2DParticlesWithData();
  void test3DCellData();
  void testAttributeTypes();
  void testMultipleSubdomains();

  /**
   * A 2-component field is written with 3 components, the third one zero, and labelled
   * AttributeType="Vector", and a 9-component field is labelled AttributeType="Tensor".
   */
  void testVectorAndTensorPadding();

  /**
   * The collective write completes when only the global master contributes data and
   * every other rank stays empty.
   */
  void testHeterogeneousRanks();

  /** An untouched plotter writes a valid empty file. */
  void testEmptyPlotter();

  /** The plotter throws when a data field does not match the geometry size. */
  void testInconsistentDataSizeThrows();

  /**
   * The plotter rejects a data identifier that carries a character which would corrupt
   * an XML attribute or an HDF5 path.
   */
  void testInvalidIdentifierThrows();

  /**
   * The collective write completes when the ranks hold different non-zero subdomain
   * counts, and the merged grid carries the summed cell total.
   */
  void testUnevenSubdomainCounts();
};
