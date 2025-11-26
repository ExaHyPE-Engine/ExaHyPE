// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::stacks::tests {
  class TreeToStacksTest;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TreeToStacksTest: public tarch::tests::TestCase {
private:
  void testGetForPush();
  void testNoRecreateOnGetForPush();
  void testGetForPop();
  void testEmpty();
  void testSize();
  void testClearAll();
  void testClearTree();
  void testHoldsStack();
  void testGarbageCollection();
  void testGetKeys();

public:
  TreeToStacksTest();
  virtual ~TreeToStacksTest() override = default;

  virtual void run() override;
};
