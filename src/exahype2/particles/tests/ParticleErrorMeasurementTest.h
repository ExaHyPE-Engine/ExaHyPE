// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class ParticleErrorMeasurementTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::ParticleErrorMeasurementTest final : public tarch::tests::TestCase {
public:
  ParticleErrorMeasurementTest();
  ParticleErrorMeasurementTest(const ParticleErrorMeasurementTest&)            = delete;
  ParticleErrorMeasurementTest(ParticleErrorMeasurementTest&&)                 = delete;
  ParticleErrorMeasurementTest& operator=(const ParticleErrorMeasurementTest&) = delete;
  ParticleErrorMeasurementTest& operator=(ParticleErrorMeasurementTest&&)      = delete;
  ~ParticleErrorMeasurementTest() override                                     = default;

  void run() override;

private:
  void testTheNormsSumThePositionErrorsOfEveryRank();
};
