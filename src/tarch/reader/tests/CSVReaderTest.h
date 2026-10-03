// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class CSVReaderTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::CSVReaderTest final : public tarch::tests::TestCase {
public:
  CSVReaderTest();
  CSVReaderTest(const CSVReaderTest&)            = delete;
  CSVReaderTest(CSVReaderTest&&)                 = delete;
  CSVReaderTest& operator=(const CSVReaderTest&) = delete;
  CSVReaderTest& operator=(CSVReaderTest&&)      = delete;
  ~CSVReaderTest() override                      = default;

  void run() override;

private:
  void testTheReaderKeepsTheRowsAfterTheSkippedLines();
  void testEveryEntryOfARowParses();
  void testAMissingFileThrows();
};
