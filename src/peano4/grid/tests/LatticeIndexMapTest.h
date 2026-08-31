// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class LatticeIndexMapTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::LatticeIndexMapTest final : public tarch::tests::TestCase {
public:
  LatticeIndexMapTest();
  LatticeIndexMapTest(const LatticeIndexMapTest&)            = delete;
  LatticeIndexMapTest(LatticeIndexMapTest&&)                 = delete;
  LatticeIndexMapTest& operator=(const LatticeIndexMapTest&) = delete;
  LatticeIndexMapTest& operator=(LatticeIndexMapTest&&)      = delete;
  ~LatticeIndexMapTest() override                            = default;

  void run() override;

private:
  void testEmptyMapHoldsNothing();
  void testInsertReportsWhetherItStored();
  void testEveryStoredKeyIsFound();
  void testClusteredKeysCollide();
  void testMapGrowsBeyondItsReservation();
  void testForEachEntryVisitsEveryEntryOnce();
  void testClearReleasesEveryEntry();
  void testCopyAndMoveCarryTheEntries();
  void testEraseReportsWhetherItRemoved();
  void testProbeChainsSurviveAnErase();
  void testErasedSlotsTakeEntriesAgain();
};
