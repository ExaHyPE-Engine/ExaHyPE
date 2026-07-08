// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class PeriodicTopologyTest;
} // namespace peano4::grid::tests

/**
 * Unit tests for the integer periodic identity (keys, stamps), the partner
 * matching, and the routing resolution (peano4/grid/PeriodicTopology.{h,cpp}).
 * These run identically in debug and release builds and do not depend on
 * GridVertex coordinates.
 */
class peano4::grid::tests::PeriodicTopologyTest final : public tarch::tests::TestCase {
private:
  void testCellIndexIdentity();
  void testCellIndexFromState();
  void testFaceStampPairing();
  void testVertexStampPairing();
  void testFaceAndVertexKeysStayDistinct();
  void testRouting();
  void testPackRoundTrip();
  void testKeyBrokerIsDeterministicAndBalanced();

public:
  PeriodicTopologyTest();
  ~PeriodicTopologyTest() override = default;

  void run() override;
};
