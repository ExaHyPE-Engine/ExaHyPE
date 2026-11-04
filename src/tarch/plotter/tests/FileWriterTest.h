// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class FileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::FileWriterTest final : public tarch::tests::TestCase {
public:
  FileWriterTest();
  FileWriterTest(const FileWriterTest& other)            = delete;
  FileWriterTest(FileWriterTest&& other)                 = delete;
  FileWriterTest& operator=(const FileWriterTest& other) = delete;
  FileWriterTest& operator=(FileWriterTest&& other)      = delete;
  ~FileWriterTest() override                             = default;

  void run() override;

private:
  void testTheTimeStampReadsBackExactly();
};
