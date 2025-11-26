// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::stacks::tests {
  class StackTest;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::StackTest: public tarch::tests::TestCase {
private:
  void testPushData();
  void testEraseData();
  void testClearData();
  void testEmplaceData();
  void testPopData();
  void testClone();
  void testPushBlockView();
  void testPopBlockView();
  void testMPIDatatype();
  void testMPIDatatype2();

public:
  StackTest();
  virtual ~StackTest() override;

  virtual void run() override;
};
