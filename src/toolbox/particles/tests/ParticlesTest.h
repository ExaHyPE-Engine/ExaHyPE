// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::particles::tests {
  class ParticlesTest;
} // namespace toolbox::particles::tests

class toolbox::particles::tests::ParticlesTest final : public tarch::tests::TestCase {
public:
  ParticlesTest();
  ParticlesTest(const ParticlesTest&)            = delete;
  ParticlesTest(ParticlesTest&&)                 = delete;
  ParticlesTest& operator=(const ParticlesTest&) = delete;
  ParticlesTest& operator=(ParticlesTest&&)      = delete;
  ~ParticlesTest() override                      = default;

  void run() override;

private:
  void testPeriodicImageTowards();
  void testApplyPeriodicBoundaryConditions();
};
