// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class SpreadDomainTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the ring of cells that joins the subcell scheme and the subgrid each of them starts from.
 */
class exahype2::dg::limiting::tests::SpreadDomainTest final : public tarch::tests::TestCase {
public:
  SpreadDomainTest();
  SpreadDomainTest(const SpreadDomainTest&)            = delete;
  SpreadDomainTest(SpreadDomainTest&&)                 = delete;
  SpreadDomainTest& operator=(const SpreadDomainTest&) = delete;
  SpreadDomainTest& operator=(SpreadDomainTest&&)      = delete;
  ~SpreadDomainTest() override                         = default;

  void run() override;

private:
  void testCellWithoutTroubledNeighbourKeepsItsSubgrid();
  void testTroubledNeighbourProjectsTheAcceptedPolynomialOntoTheSubgrid();
  void testCellEnteringWithinTheFirstStepKeepsItsSampledSubcells();
  void testBoundaryFaceHaloComesFromTheBoundaryConditions();
  void testALimitedCellKeepsItsEvolvedSubgrid();
  void testALimitedNeighbourSpreadsTheHaloStatus();
  void testAPeriodicBoundaryFaceKeepsItsOuterLayer();
};
