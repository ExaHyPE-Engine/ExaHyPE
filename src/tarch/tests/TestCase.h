// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TestMacros.h"

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
  namespace tests {
    class TestCase;

    /**
     * Directory into which tests may write scratch/fixture files.
     */
    std::filesystem::path getWritableTestDirectory();
  } // namespace tests
} // namespace tarch

/**
 * Represents one test case. Every test case should be a subclass of this
 * class implementing run(). Furthermore subtypes should use the assertion
 * macros included to check any assumption. All the test cases are managed by the
 * TestCaseCollection.
 *
 * If you implement your own tests, inherit from this class, implement
 * run() and add the test to a test case collection of your choice.
 *
 * ## Unit tests in libraries
 *
 * If your unit test is part of a library, then simply registering the test is
 * not enough, as the linker removes the test again once it looks uninvoked.
 * Every library hence builds up its unit test suite manually. See the
 * respective headers called UnitTests.
 *
 * @author Tobias Weinzierl
 */
class tarch::tests::TestCase {
protected:
  /**
   * Name of the test case.
   */
  const std::string _testCaseName;

  /**
   * Error counter. This counter is increased on errors only when you use the
   * validate macros within your test cases.
   */
  int _errors;

  /**
   * Indicate within macro if one specific validation failed.
   */
  bool _error;

  TestCase() = delete;

public:
  /**
   * Constructor.
   *
   * @param testCaseName Name of the test case. Should be a fully qualified name.
   */
  TestCase(const std::string& testCaseName);

  /**
   * Destructor.
   */
  virtual ~TestCase();

  /**
   * @return Number of errors.
   */
  int getNumberOfErrors() const;

  std::string getTestCaseName() const;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run() = 0;
};
