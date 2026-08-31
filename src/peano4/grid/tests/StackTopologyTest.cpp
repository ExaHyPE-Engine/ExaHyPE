// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopologyTest.h"

#include "peano4/grid/StackTopology.h"

peano4::grid::tests::StackTopologyTest::StackTopologyTest() :
  TestCase("peano4::grid::tests::StackTopologyTest") {}

void peano4::grid::tests::StackTopologyTest::run() { testMethod(testStackIdentification); }

void peano4::grid::tests::StackTopologyTest::testStackIdentification() {
  validate(
    not peano4::grid::StackTopology::isInOutStack(static_cast<int>(peano4::grid::StackTopology::BaseStack::Call))
  );
  validate(peano4::grid::StackTopology::isInOutStack(static_cast<int>(peano4::grid::StackTopology::BaseStack::Input)));
}
