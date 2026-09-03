// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class LatticeTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::LatticeTest final : public tarch::tests::TestCase {
private:
  /** Keys and lattice positions convert back and forth without loss. */
  void testTriadicMortonKeyRoundTrip();

  /** Cells below one ancestor form one contiguous key range. */
  void testAncestorKeysArePrefixes();

  /** A regular grid's block enumeration lists every cell of the block in ascending key order. */
  void testLeafEnumerationOnRegularGrid();

public:
  LatticeTest();
  ~LatticeTest() override = default;

  void run() override;
};
