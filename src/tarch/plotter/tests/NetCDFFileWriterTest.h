// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class NetCDFFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::NetCDFFileWriterTest final : public tarch::tests::TestCase {
public:
  NetCDFFileWriterTest();
  NetCDFFileWriterTest(const NetCDFFileWriterTest& other)            = delete;
  NetCDFFileWriterTest(NetCDFFileWriterTest&& other)                 = delete;
  NetCDFFileWriterTest& operator=(const NetCDFFileWriterTest& other) = delete;
  NetCDFFileWriterTest& operator=(NetCDFFileWriterTest&& other)      = delete;
  ~NetCDFFileWriterTest() override                                   = default;

  void run() override;

private:
  void test2DQuadrangleWithData();
  void test3DHexahedronWithData();
  void testTwoQuadrangles2DWithData();
  void testHeterogeneousRanks();
  void testUnevenCellCounts();
  void testEmptyMesh();
  void testSeries();
};
