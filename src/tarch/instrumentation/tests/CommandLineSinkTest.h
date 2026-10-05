// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::instrumentation::tests {
  class CommandLineSinkTest;
} // namespace tarch::instrumentation::tests

class tarch::instrumentation::tests::CommandLineSinkTest final : public tarch::tests::TestCase {
public:
  CommandLineSinkTest();
  CommandLineSinkTest(const CommandLineSinkTest&)            = delete;
  CommandLineSinkTest(CommandLineSinkTest&&)                 = delete;
  CommandLineSinkTest& operator=(const CommandLineSinkTest&) = delete;
  CommandLineSinkTest& operator=(CommandLineSinkTest&&)      = delete;
  ~CommandLineSinkTest() override                            = default;

  void run() override;

private:
  void testAPlainMessageReachesItsStreamUnchanged();
};
