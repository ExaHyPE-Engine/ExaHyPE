// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class NodeTest;
} // namespace peano4::parallel::tests

class peano4::parallel::tests::NodeTest final : public tarch::tests::TestCase {
public:
  NodeTest();
  void run() override;

private:
  void testGetPeriodicBoundaryNumber();
  void testBitsetBasedPeriodicBoundaryEnumeration();
  void testGetOutputStacksForPeriodicBoundaryExchange();
  void testTagCalculation();
  void testDataExchangeCommunicatorCreation();
};
