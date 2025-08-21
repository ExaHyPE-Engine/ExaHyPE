// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/UnitTests.h"
#include "peano4/peano4.h"
#include "tarch/UnitTests.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"

#ifdef ENABLE_TOOLBOX_BLOCKSTRUCTURED
#include "toolbox/blockstructured/UnitTests.h"
#endif

#ifdef ENABLE_TOOLBOX_PARTICLES
#include "toolbox/particles/UnitTests.h"
#endif

#ifdef ENABLE_EXAHYPE
#include "exahype2/UnitTests.h"
#endif

#if defined(__linux)
#include <cfenv>
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
#endif

tarch::logging::Log _log("unittests");

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
    logError("main()", unitTestsErrors << " unit tests failed. Quit.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      0,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      0,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      0,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      0,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

#if DIMENSIONS == 2
  const auto DomainOffset = {0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0};
#else
  const auto DomainOffset = {0.0, 0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0, 1.0};
#endif

  peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);

#if defined(__linux)
  // feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  int returnCode = runTests();

  peano4::shutdown();

  return returnCode;
}
