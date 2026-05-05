// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/UnitTests.h"
#include "peano4/peano4.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/UnitTests.h"
#include "tarch/instrumentation/Log.h"

#ifdef ENABLE_TOOLBOX_BLOCKSTRUCTURED
#include "toolbox/blockstructured/UnitTests.h"
#endif

#ifdef ENABLE_TOOLBOX_LOADBALANCING
#include "toolbox/loadbalancing/UnitTests.h"
#endif

#ifdef ENABLE_TOOLBOX_PARTICLES
#include "toolbox/particles/UnitTests.h"
#endif

#ifdef ENABLE_EXAHYPE
#include "exahype2/UnitTests.h"
#endif

int runTests() {
  int                     unitTestsErrors = 0;
  tarch::tests::TestCase* tests           = nullptr;

  tests = tarch::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = peano4::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

#ifdef ENABLE_TOOLBOX_BLOCKSTRUCTURED
  tests = toolbox::blockstructured::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef ENABLE_TOOLBOX_LOADBALANCING
  tests = toolbox::loadbalancing::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef ENABLE_TOOLBOX_PARTICLES
  tests = toolbox::particles::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef ENABLE_EXAHYPE
  tests = exahype2::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

  if (unitTestsErrors != 0) {
    logError(unitTestsErrors << " unit tests failed. Quit.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
#if DIMENSIONS == 2
  const auto DomainOffset = {0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0};
#else
  const auto DomainOffset = {0.0, 0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0, 1.0};
#endif

  peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);

  int returnCode = EXIT_SUCCESS;

#if defined(WITH_OPENMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      {
        tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
        returnCode = runTests();
      }

      if (tarch::hasNonCriticalAssertionBeenViolated()) {
        logError("Noncritical assertion has been triggered while running the unit tests. Quit.");
        returnCode = EXIT_FAILURE;
      }
#if defined(WITH_OPENMP)
    }
  }
#endif

  peano4::shutdown();

  return returnCode;
}
