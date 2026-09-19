// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class DynamicAMRTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks how the subcell buffers and the limiter state of a cell cross a level jump.
 */
class exahype2::dg::limiting::tests::DynamicAMRTest final : public tarch::tests::TestCase {
public:
  DynamicAMRTest();
  DynamicAMRTest(const DynamicAMRTest&)            = delete;
  DynamicAMRTest(DynamicAMRTest&&)                 = delete;
  DynamicAMRTest& operator=(const DynamicAMRTest&) = delete;
  DynamicAMRTest& operator=(DynamicAMRTest&&)      = delete;
  ~DynamicAMRTest() override                       = default;

  void run() override;

private:
  void testResetCoarseningParentLabelActsOnACoarseningParentOnly();
  void testCreateCellProlongsTheSubcellsOfALimitedParent();
  void testCreateCellProjectsTheNodalSolutionOfARegularParent();
  void testDestroyCellRestrictsAMixedSiblingSetAndMergesTheLimiterState();
};
