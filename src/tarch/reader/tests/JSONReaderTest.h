// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class JSONReaderTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::JSONReaderTest : public tarch::tests::TestCase {
private:
  void testReadPrimitivesAndArrays();
  void testFailureModes();
  void testUnusedKeys();

public:
  JSONReaderTest();
  ~JSONReaderTest() override = default;

  void run() override;
  void setUp() {}
};
