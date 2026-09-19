// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SubcellLayoutTest.h"

#include "exahype2/dg/limiting/SubcellLayout.h"
#include "peano4/Constants.h"

namespace {
  struct OrderThreeSolver final {
    static constexpr int DGOrder                    = 3;
    static constexpr int NumberOfUnknowns           = 4;
    static constexpr int NumberOfAuxiliaryVariables = 1;
  };
} // namespace

exahype2::dg::limiting::tests::SubcellLayoutTest::SubcellLayoutTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::SubcellLayoutTest") {}

void exahype2::dg::limiting::tests::SubcellLayoutTest::run() { testMethod(testTheCountsFollowTheOrderAndTheUnknowns); }

/**
 * Checks that an order-three solver with four unknowns and one auxiliary variable has four nodes
 * and seven subcells per axis and five values per node.
 */
void exahype2::dg::limiting::tests::SubcellLayoutTest::testTheCountsFollowTheOrderAndTheUnknowns() {
  using Layout = exahype2::dg::limiting::SubcellLayout<OrderThreeSolver>;

  validateEquals(Layout::NumberOfDofsPerAxis, 4);
  validateEquals(Layout::NumberOfSubcellsPerAxis, 7);
  validateEquals(Layout::NumberOfData, 5);
  validateEquals(Layout::NumberOfNodes, peano4::power(4, DIMENSIONS));
  validateEquals(Layout::NumberOfSubcells, peano4::power(7, DIMENSIONS));
  validateEquals((exahype2::dg::limiting::ValuesPerSubcellPatch<7, 5>), 5 * peano4::power(7, DIMENSIONS));
}
