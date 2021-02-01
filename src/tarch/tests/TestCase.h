// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_TESTS_TESTCASE_H_
#define _TARCH_TESTS_TESTCASE_H_

#include "tarch/tests/TestMacros.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include <string>


namespace tarch {
  namespace tests {
    class TestCase;
  }
}


/**
 * Represents one test case. Every test case should be a subclass of this
 * class implementing run(). Furthermore subtypes should use the assertion
 * macros included to check any assumption. All the test cases are managed by the
 * TestCaseCollection.
 *
 * If you implement your own tests, inherit from this class, implement
 * run() and add the test to a test case collection of your choice.
 *
 *
 * <h2> Implementation remarks </h2>
 *
 * Whenever you write a test case implementation, we recommend to add the
 * code block
 *
 * \code
#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif
   \endcode
 * at the very beginning of your implementation and
 * \code
#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
   \endcode
 *
 * at the end of the implementation file.
 *
 *
 * <h2> Unit tests in libraries </h2>
 *
 * If your unit test is part of a library, then simply registering the test is
 * not enough, as the linker will remove the test again - assuming it is not
 * invoked. I therefore decided not to rely on any automatic test case suites
 * or so, but instead let the individual libraries build up their unit test
 * suites manually. See the respective headers called UnitTests.
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
     * Error counter. This counter is increased on errors iff you use the
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
    TestCase( const std::string& testCaseName );

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

#endif
