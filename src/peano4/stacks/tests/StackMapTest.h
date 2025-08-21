// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::stacks::tests {
  class StackMapTest;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::StackMapTest: public tarch::tests::TestCase {
private:
  void testGetForPush();
  void testGetForPop();
  void testEmpty();
  void testClear();
  void testGarbageCollection();
  void testGetKeys();

public:
  StackMapTest();
  virtual ~StackMapTest() override = default;

  virtual void run() override;
};
