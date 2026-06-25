// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TestMacros.h"
#include "tarch/CompilerSpecificSettings.h"

#include <filesystem>
#include <string>

namespace tarch {
  namespace tests {
    class TestCase;

    /**
     * Directory into which tests may write scratch/fixture files.
     *
     * Any test that needs to create a file on disk should obtain its target
     * location through this function rather than hard-coding "./" or
     * std::filesystem::temp_directory_path(). On the shared CI nodes the system
     * temp directory ($TMPDIR // /tmp) is not reliably writable for a job (a
     * per-job scratch dir that was already cleaned, restrictive permissions, or
     * a stale fixture owned by another user), which intermittently made
     * file-writing tests fail with "Permission denied" - and, because such a
     * failure typically escapes the test as an exception, abort the whole unit
     * test binary.
     *
     * This helper returns a directory that is verified to be writable. It is the
     * same directory on every MPI rank, which is essential: the parallel
     * plotters write collectively to a single shared file (nc_create_par,
     * H5Pset_fapl_mpio), so a per-rank path would deadlock the collective open.
     * It therefore prefers the current working directory (identical for all
     * ranks of a run, owned by the job, hence writable, and isolated per job)
     * and only falls back to the system temp directory if the working directory
     * is not writable. The result is determined once and cached.
     *
     * Combine it with the desired file name via std::filesystem::path, e.g.
     * `getWritableTestDirectory() / "my-fixture.nc"`.
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
