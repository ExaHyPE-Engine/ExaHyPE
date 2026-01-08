// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class HDF5FileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::HDF5FileWriterTest final: public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test2DParticlesWithData();
  void test3DCellData();
  void testAttributeTypes();

  /**
   * Checks if an untouched plotter produces a valid empty file.
   */
  void testEmptyPlotter();

  /**
   * Verifies that the plotter throws an exception if we try to write
   * data fields that do not match the geometry size (strict check).
   */
  void testInconsistentDataSizeThrows();

public:
  HDF5FileWriterTest();
  ~HDF5FileWriterTest() override = default;

  void run() override;
};
