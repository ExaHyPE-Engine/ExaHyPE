// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/tests/TestMacros.h"

namespace tarch::tests {
  class TestCase;

  std::filesystem::path getWritableTestDirectory();
} // namespace tarch::tests

/**
 * Base class of one test case. A subclass implements run() and checks its
 * assumptions with the validate macros of TestMacros.h.
 *
 * A unit test of a library is registered by that library's own test suite, as
 * the linker drops a test that no translation unit calls.
 *
 * @author Tobias Weinzierl
 */
class tarch::tests::TestCase {
public:
  TestCase() = delete;

  TestCase(const std::string& testCaseName);

  virtual ~TestCase();

  int getNumberOfErrors() const;

  std::string getTestCaseName() const;

  virtual void run() = 0;

protected:
  const std::string _testCaseName;

  /**
   * The validate macros add one per failed check.
   */
  int _errors{0};

  /**
   * The validate macros set this flag when one check fails.
   */
  bool _error{false};
};
