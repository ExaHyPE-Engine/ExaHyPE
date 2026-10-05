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
  HDF5FileWriterTest(const HDF5FileWriterTest& other)            = delete;
  HDF5FileWriterTest(HDF5FileWriterTest&& other)                 = delete;
  HDF5FileWriterTest& operator=(const HDF5FileWriterTest& other) = delete;
  HDF5FileWriterTest& operator=(HDF5FileWriterTest&& other)      = delete;
  ~HDF5FileWriterTest() override                                 = default;

  void run() override;

private:
  void test2DSimpleQuadrangle();
  void test2DParticlesWithData();
  void test3DCellData();
  void testAttributeTypes();
  void testMixedCellTypes();
  void testVectorAndTensorPadding();
  void testHeterogeneousRanks();
  void testEmptyMesh();
  void testUnevenCellCounts();
  void testSeries();
};
