// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class TreeIdMappingTest;
} // namespace peano4::parallel::tests

class peano4::parallel::tests::TreeIdMappingTest final : public tarch::tests::TestCase {
public:
  TreeIdMappingTest();
  void run() override;

private:
  void testIdArithmetic();
  void testReserveAndDeregister();
};
