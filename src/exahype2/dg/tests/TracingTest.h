// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class TracingTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::TracingTest final : public tarch::tests::TestCase {
public:
  TracingTest();
  TracingTest(const TracingTest&)            = delete;
  TracingTest(TracingTest&&)                 = delete;
  TracingTest& operator=(const TracingTest&) = delete;
  TracingTest& operator=(TracingTest&&)      = delete;
  ~TracingTest() override                    = default;

  void run() override;

private:
  void testInterpolateFieldsWritesThePolynomialIntoTheNamedEntries();
  void testTracingCarriesAParticleWithTheFluid();
  void testStaticSamplingPreservesParticleData();
  void testFaceTracingWritesTheMeanAndTheJumpOfTheTraces();
};
