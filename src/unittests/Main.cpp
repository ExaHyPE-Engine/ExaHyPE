// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/UnitTests.h"
#include "peano4/Init.h"
#include "peano4/UnitTests.h"
#include "runner/UnitTests.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/UnitTests.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "toolbox/blockstructured/UnitTests.h"
#include "toolbox/loadbalancing/UnitTests.h"
#include "toolbox/particles/UnitTests.h"

namespace {
  int runTests() {
    int numberOfErrors = 0;

    const auto runTestCollection = [&numberOfErrors](tarch::tests::TestCase* const testCollection) {
      const std::unique_ptr<tarch::tests::TestCase> ownedTestCollection{testCollection};
      ownedTestCollection->run();
      numberOfErrors += ownedTestCollection->getNumberOfErrors();
    };

    runTestCollection(tarch::getUnitTests());
    runTestCollection(peano4::getUnitTests());
    runTestCollection(toolbox::blockstructured::getUnitTests());
    runTestCollection(toolbox::loadbalancing::getUnitTests());
    runTestCollection(toolbox::particles::getUnitTests());
    runTestCollection(exahype2::getUnitTests());
    runTestCollection(runner::getUnitTests());

    if (numberOfErrors != 0) {
      logError("Unit tests failed: " << numberOfErrors << ".");
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }
} // namespace

int main(int argc, char** argv) {
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0.0, 0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {1.0, 1.0, 1.0};
#endif

  // No timeout warning: the runner silences warnings, so only the abort speaks.
  tarch::init(&argc, &argv, {0.0, 60.0, true});
  // Several tests intentionally exercise warning paths. Keep the test runner
  // silent on success while retaining errors and its non-zero failure status.
  tarch::instrumentation::Log::setInfoOutputEnabled(false);
  tarch::instrumentation::Log::setWarningOutputEnabled(false);
  // Every thread carries its own floating-point environment, so the traps are armed
  // once here, once per worker inside the parallel region, and once around the tests
  // so that the environment is restored afterwards. This one is dropped before
  // tarch::shutdown(), because finalising MPI is outside our control and may compute
  // with values we would abort on.
  std::optional<tarch::FloatingPointExceptions::ScopedTrap> testProcessTrap(std::in_place, __FILE__, __LINE__);
  peano4::init(domainOffset, domainSize);

  int returnCode = EXIT_SUCCESS;

#if defined(PEANO_WITH_OPENMP)
#pragma omp parallel
#endif
  {
    const tarch::FloatingPointExceptions::ScopedTrap threadTrap(__FILE__, __LINE__);
#if defined(PEANO_WITH_OPENMP)
#pragma omp barrier
#pragma omp master
    {
#endif
      {
        const tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
        returnCode = runTests();
      }

      if (tarch::hasNonCriticalAssertionBeenViolated()) {
        logError("Non-critical assertion has been triggered while running the unit tests. Quit.");
        returnCode = EXIT_FAILURE;
      }
#if defined(PEANO_WITH_OPENMP)
    }
#pragma omp barrier
#endif
  }

  peano4::shutdown();
  testProcessTrap.reset();
  tarch::shutdown();
  return returnCode;
}
