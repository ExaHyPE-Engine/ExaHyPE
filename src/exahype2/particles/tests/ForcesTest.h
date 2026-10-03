// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class ForcesTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::ForcesTest final : public tarch::tests::TestCase {
public:
  ForcesTest();
  ForcesTest(const ForcesTest&)            = delete;
  ForcesTest(ForcesTest&&)                 = delete;
  ForcesTest& operator=(const ForcesTest&) = delete;
  ForcesTest& operator=(ForcesTest&&)      = delete;
  ~ForcesTest() override                   = default;

  void run() override;

private:
  void testAMasslessParticleCarriesNoAcceleration();
  void testEveryForceWritesItsAcceleration();
  void testTheEulerSchemesAdvanceOneStep();
  void testTheHigherOrderSchemesAreExactForAConstantAcceleration();
  void testHigherOrderSchemesConvergeForDrag();
  void testStaticIntegrationNeedsNoDataAttributes();
  void testReflectionAndWrapAtTheWalls();
};
