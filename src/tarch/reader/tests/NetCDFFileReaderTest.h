// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class NetCDFFileReaderTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::NetCDFFileReaderTest: public tarch::tests::TestCase {
private:
  void testOpenReadAndClose();
  void testMissingFileFails();

public:
  NetCDFFileReaderTest();
  ~NetCDFFileReaderTest() override = default;

  void run() override;
};
