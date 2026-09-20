// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class GridAssociationTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::GridAssociationTest final : public tarch::tests::TestCase {
public:
  GridAssociationTest();
  GridAssociationTest(const GridAssociationTest&)            = delete;
  GridAssociationTest(GridAssociationTest&&)                 = delete;
  GridAssociationTest& operator=(const GridAssociationTest&) = delete;
  GridAssociationTest& operator=(GridAssociationTest&&)      = delete;
  ~GridAssociationTest() override                            = default;

  void run() override;

private:
  void testStatisticsClearAndRecord();
  void testDropParticlesOntoTheFineVertex();
  void testDropParticlesIntoTheSieveSetOfASubtree();
  void testSieveParticlesIntoVertex();
  void testLiftOrReassignKeepsAParticleWithinReach();
  void testLiftOrReassignWrapsAParticleAtAPeriodicWall();
  void testLiftOrReassignLiftsAParticleToTheParentVertex();
  void testLiftOrReassignSievesAParticleGlobally();
  void testLiftAllParticlesToTheParentVertex();
  void testLiftAllParticlesIntoTheSieveSet();
};
