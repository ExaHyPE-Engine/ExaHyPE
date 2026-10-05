// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTUFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::VTUFileWriterTest final : public tarch::tests::TestCase {
public:
  VTUFileWriterTest();
  VTUFileWriterTest(const VTUFileWriterTest& other)            = delete;
  VTUFileWriterTest(VTUFileWriterTest&& other)                 = delete;
  VTUFileWriterTest& operator=(const VTUFileWriterTest& other) = delete;
  VTUFileWriterTest& operator=(VTUFileWriterTest&& other)      = delete;
  ~VTUFileWriterTest() override                                = default;

  void run() override;

private:
  void test2DSimpleQuadrangle();
  void test3DSimpleHexahedron();
  void test2DParticlesWithData();
  void test3DCellData();
  void testEmptyMesh();
  void testSeries();
};
