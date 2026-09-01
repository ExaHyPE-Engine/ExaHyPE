// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class VTKSamplerTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::VTKSamplerTest: public tarch::tests::TestCase {
private:
  // The VTKHDF back-end, i.e. one growing file holding all steps.
  void testSampleVertexData();
  void testSampleClosestVertex();
  void testSelectStep();
  void testSampleCellData();
  void testUnknownFieldThrows();

  // The VTU back-end, i.e. a .pvd collection of per-step .pvtu files.
  void testSampleVertexDataFromVTU();
  void testSampleClosestVertexFromVTU();
  void testSelectStepFromVTU();
  void testSampleCellDataFromVTU();
  void testSampleSeveralPiecesFromVTU();
  void testReadSnapshotFileFromVTU();
  void testUnknownFieldThrowsInVTU();
  void testSampleAsciiVTU();
  void testSampleBase64VTU();

  void testUnknownFormatThrows();

public:
  VTKSamplerTest();
  ~VTKSamplerTest() override = default;

  void run() override;
};
