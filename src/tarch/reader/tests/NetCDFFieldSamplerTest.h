// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class NetCDFFieldSamplerTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::NetCDFFieldSamplerTest final : public tarch::tests::TestCase {
public:
  NetCDFFieldSamplerTest();
  NetCDFFieldSamplerTest(const NetCDFFieldSamplerTest& other)            = delete;
  NetCDFFieldSamplerTest(NetCDFFieldSamplerTest&& other)                 = delete;
  NetCDFFieldSamplerTest& operator=(const NetCDFFieldSamplerTest& other) = delete;
  NetCDFFieldSamplerTest& operator=(NetCDFFieldSamplerTest&& other)      = delete;
  ~NetCDFFieldSamplerTest() override                                     = default;

  void run() override;

private:
  void testPiecewiseConstantSampleSingleField();
  void testOutOfDomainReturnsZero();
  void testCoversTheStoredExtentOnly();
  void testMultipleNamedFields();
  void testDisplacementUsesItsOwnCoordinateRange();
  void testSamplingWithSimulationOffset();
  void testSamplingWithFileExtent();
  void testBilinearSampling();
  void testThreeDimensionalSampling();
  void testThreeDimensionalBilinearSampling();
};
