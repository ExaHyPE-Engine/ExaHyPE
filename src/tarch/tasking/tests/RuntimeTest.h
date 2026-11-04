// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tasking::tests {
  class RuntimeTest;
} // namespace tarch::tasking::tests

class tarch::tasking::tests::RuntimeTest final : public tarch::tests::TestCase {
public:
  RuntimeTest();
  RuntimeTest(const RuntimeTest&)            = delete;
  RuntimeTest(RuntimeTest&&)                 = delete;
  RuntimeTest& operator=(const RuntimeTest&) = delete;
  RuntimeTest& operator=(RuntimeTest&&)      = delete;
  ~RuntimeTest() override                    = default;

  void run() override;

private:
  void testRuntimeProperties();
};
