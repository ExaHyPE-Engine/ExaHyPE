// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestCase.h"

#include <unistd.h>

namespace {
  std::filesystem::path determineWritableTestDirectory() {
    std::error_code             ec;
    const std::filesystem::path candidates[]
      = {std::filesystem::current_path(ec), std::filesystem::temp_directory_path(ec)};

    for (const auto& base : candidates) {
      if (base.empty()) {
        continue;
      }
      // Probe with a per-process file name so concurrent ranks do not race on
      // the probe, but hand back the shared base directory itself.
      const std::filesystem::path probe = base / (".peano-write-probe-" + std::to_string(::getpid()));
      std::ofstream               stream(probe);
      if (stream.good()) {
        stream.close();
        std::filesystem::remove(probe, ec);
        return base;
      }
    }

    // Last resort: the current working directory, which the job owns.
    return std::filesystem::current_path(ec);
  }
} // namespace

/**
 * Directory into which tests may write scratch/fixture files.
 *
 * Any test which needs to create a file on disk should obtain its target
 * location through this function rather than hard-coding "./" or
 * std::filesystem::temp_directory_path(). On the shared CI nodes, the system
 * temp directory ($TMPDIR // /tmp) is not reliably writable for a job: a
 * per-job scratch directory may already have been cleaned up, the permissions
 * may be restrictive, or a stale fixture may belong to another user. Opening
 * a file there hence fails intermittently, and as such a failure typically
 * escapes the test as an exception, it takes the whole unit test binary down.
 *
 * This helper returns a directory which is verified to be writable. It is the
 * same directory on every MPI rank, which is essential: the parallel plotters
 * write collectively to a single shared file (nc_create_par,
 * H5Pset_fapl_mpio), so a per-rank path would deadlock the collective open.
 * It therefore prefers the current working directory, which is identical for
 * all ranks of a run, owned by the job, hence writable, and isolated per job.
 * Only if that directory is not writable does it fall back to the system temp
 * directory. The result is determined once and cached.
 *
 * Combine it with the desired file name via std::filesystem::path, e.g.
 * `getWritableTestDirectory() / "my-fixture.nc"`.
 */
std::filesystem::path tarch::tests::getWritableTestDirectory() {
  // Magic-static initialisation: thread-safe and evaluated only once.
  static const std::filesystem::path directory = determineWritableTestDirectory();
  return directory;
}

tarch::tests::TestCase::TestCase(const std::string& testCaseName):
  _testCaseName(testCaseName),
  _errors(0),
  _error(false) {}

tarch::tests::TestCase::~TestCase() {}

int tarch::tests::TestCase::getNumberOfErrors() const { return _errors; }

std::string tarch::tests::TestCase::getTestCaseName() const { return _testCaseName; }
