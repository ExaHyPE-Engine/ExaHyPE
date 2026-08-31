// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class StackTopologyTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::StackTopologyTest final : public tarch::tests::TestCase {
public:
  StackTopologyTest();
  StackTopologyTest(const StackTopologyTest&)            = delete;
  StackTopologyTest(StackTopologyTest&&)                 = delete;
  StackTopologyTest& operator=(const StackTopologyTest&) = delete;
  StackTopologyTest& operator=(StackTopologyTest&&)      = delete;
  ~StackTopologyTest() override                          = default;

  void run() override;

private:
  void testStackIdentification();
};
