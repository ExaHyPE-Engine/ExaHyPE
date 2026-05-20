// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class StackTopologyTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::StackTopologyTest final: public tarch::tests::TestCase {
private:
  void testStackIdentification();
  void testPeriodicBoundaryMapping();

public:
  StackTopologyTest();
  ~StackTopologyTest() override final = default;

  void run() override final;
};
