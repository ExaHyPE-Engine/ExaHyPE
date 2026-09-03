// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::reader::tests {
  class HDF5FileReaderTest;
} // namespace tarch::reader::tests

class tarch::reader::tests::HDF5FileReaderTest final : public tarch::tests::TestCase {
private:
  /** Row ranges of double, int32 and int64 datasets read back the written values. */
  void testReadRowRanges();

  /** datasetExists() answers for present and absent paths without an error. */
  void testDatasetExists();

public:
  HDF5FileReaderTest();
  ~HDF5FileReaderTest() override = default;

  void run() override;
};
