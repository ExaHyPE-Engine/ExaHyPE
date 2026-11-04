// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class TracingTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::TracingTest final : public tarch::tests::TestCase {
public:
  TracingTest();
  TracingTest(const TracingTest& other)            = delete;
  TracingTest(TracingTest&& other)                 = delete;
  TracingTest& operator=(const TracingTest& other) = delete;
  TracingTest& operator=(TracingTest&& other)      = delete;
  ~TracingTest() override                          = default;

  void run() override;

private:
  void testPiecewiseConstantReadsTheVolumeThatContainsThePosition();
  void testPiecewiseLinearReproducesALinearField();
  void testPiecewiseLinearReturnsTheSameBitsAtMirroredPositions();
  void testInterpolationOnOneVolumePatches();
  void testInterpolationOnRectangularPatches();
  void testClampIntoPatchProjectsOntoTheVolumeCentres();
  void testTracingCarriesAParticleWithTheFluid();
  void testStaticSamplingPreservesParticleData();
};
