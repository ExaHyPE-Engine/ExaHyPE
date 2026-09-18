// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class JSONReaderTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::JSONReaderTest final : public tarch::tests::TestCase {
public:
  JSONReaderTest();
  JSONReaderTest(const JSONReaderTest&)            = delete;
  JSONReaderTest(JSONReaderTest&&)                 = delete;
  JSONReaderTest& operator=(const JSONReaderTest&) = delete;
  JSONReaderTest& operator=(JSONReaderTest&&)      = delete;
  ~JSONReaderTest() override                       = default;

  void run() override;

private:
  void testReadPrimitivesAndArrays();
  void testUnusedKeys();
};
