// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class NetCDFFieldSamplerTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::NetCDFFieldSamplerTest : public tarch::tests::TestCase {
private:
  void testPiecewiseConstantSampleSingleField();
  void testOutOfDomainReturnsZero();
  void testMultipleNamedFields();
  void testDisplacementUsesItsOwnCoordinateRange();
  void testSamplingWithSimulationOffset();
  void testSamplingWithFileExtent();
  void testBilinearSampling();
  void testThreeDimensionalSampling();
  void testThreeDimensionalBilinearSampling();

public:
  NetCDFFieldSamplerTest();
  ~NetCDFFieldSamplerTest() override = default;

  void run() override;
};
