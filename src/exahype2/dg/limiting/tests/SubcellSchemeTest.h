// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class SubcellSchemeTest;
} // namespace exahype2::dg::limiting::tests

class exahype2::dg::limiting::tests::SubcellSchemeTest final : public tarch::tests::TestCase {
public:
  SubcellSchemeTest();
  SubcellSchemeTest(const SubcellSchemeTest&)            = delete;
  SubcellSchemeTest(SubcellSchemeTest&&)                 = delete;
  SubcellSchemeTest& operator=(const SubcellSchemeTest&) = delete;
  SubcellSchemeTest& operator=(SubcellSchemeTest&&)      = delete;
  ~SubcellSchemeTest() override                          = default;

  void run() override;

private:
  void testAConstantStateProjectsOntoConstantSubcellsAndBack();
  void testTheInitialConditionIsSampledAtTheSubcellCentres();
  void testEverySubcellIsVisitedAtItsCentre();
  void testTheSubcellWaveSpeedsScaleWithTheCellSize();
  void testTheTermsForwardOnlyWhatTheSolverOffers();
  void testTheRiemannSolverCallsTheSubcellRiemannSolver();
  void testAConstantSubgridRecomputesToTheConstantAndReportsItsWaveSpeed();
  void testAFixedRuleReportsNoWaveSpeeds();
  void testAGalerkinCellProjectsItsPreviousSolutionOntoTheSubgridAndTheFaces();
  void testEveryCellOfADeviceRangeRecomputesLikeTheHost();
  void testEveryNodeProjectsLikeTheWholeCell();
};
