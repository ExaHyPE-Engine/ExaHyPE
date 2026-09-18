// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::particles::tests {
  class MultiscaleTransitionsTest;
} // namespace peano4::particles::tests

class peano4::particles::tests::MultiscaleTransitionsTest final : public tarch::tests::TestCase {
public:
  MultiscaleTransitionsTest();
  MultiscaleTransitionsTest(const MultiscaleTransitionsTest&)            = delete;
  MultiscaleTransitionsTest(MultiscaleTransitionsTest&&)                 = delete;
  MultiscaleTransitionsTest& operator=(const MultiscaleTransitionsTest&) = delete;
  MultiscaleTransitionsTest& operator=(MultiscaleTransitionsTest&&)      = delete;
  ~MultiscaleTransitionsTest() override                                  = default;

  void run() override;

private:
  void testVertexKeepsAndDropsAParticleWithinItsReach();
  void testVertexLiftsAParticleBeyondItsReachToTheParentVertex();
  void testLiftSievesGloballyWithoutALocalParent();
  void testRemoteParticleIsKept();
  void testSieveWaitsForTheLevelTheParticleFitsInto();
  void testReassociationWithinCell();
  void testAdjacentCellsOwningParticle();
};
