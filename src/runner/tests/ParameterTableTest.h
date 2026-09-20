// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class ParameterTableTest;
} // namespace runner::tests

class runner::tests::ParameterTableTest final : public tarch::tests::TestCase {
public:
  ParameterTableTest();
  ParameterTableTest(const ParameterTableTest&)            = delete;
  ParameterTableTest(ParameterTableTest&&)                 = delete;
  ParameterTableTest& operator=(const ParameterTableTest&) = delete;
  ParameterTableTest& operator=(ParameterTableTest&&)      = delete;
  ~ParameterTableTest() override                           = default;

  void run() override;

private:
  void testCommandLineSetsEntriesAndKeepsTheOtherArguments();
  void testConfigurationFileSetsEntriesBeforeTheCommandLine();
  void testUsageNamesEveryEntryWithItsDefault();
};
