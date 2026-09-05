// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class CheckpointFileTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::CheckpointFileTest final : public tarch::tests::TestCase {
private:
  /** getPath() names the checkpoint after the directory and the checkpoint's number. */
  void testPath();

  /** The file stays open while a writer holds it and counts the checkpoint up once the last writer leaves. */
  void testWritersHoldOneFile();

public:
  CheckpointFileTest();
  ~CheckpointFileTest() override = default;

  void run() override;
};
