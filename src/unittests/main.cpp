// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "config.h"

#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON

#include "peano4/parallel/Node.h"
#include "peano4/peano4.h"
#include "peano4/UnitTests.h"
#include "tarch/logging/ChromeTraceFileLogger.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/UnitTests.h"

#ifdef UseToolboxBlockstructured
#include "toolbox/blockstructured/UnitTests.h"
#endif

#ifdef UseToolboxFiniteElements
#include "toolbox/finiteelements/UnitTests.h"
#endif

#ifdef UseToolboxMultiprecision
#include "toolbox/multiprecision/UnitTests.h"
#endif

#ifdef UseToolboxParticles
#include "toolbox/particles/assignmentchecks/UnitTests.h"
#include "toolbox/particles/UnitTests.h"
#endif

#ifdef UseExaHyPE
#include "exahype2/UnitTests.h"
#endif

#ifdef UseSWIFT
#include "swift2/UnitTests.h"
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

#ifdef UseToolboxBlockstructured
  tests = toolbox::blockstructured::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef UseToolboxFiniteElements
  tests = toolbox::finiteelements::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef UseToolboxMultiprecision
  tests = toolbox::multiprecision::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef UseToolboxParticles
  tests = toolbox::particles::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = toolbox::particles::assignmentchecks::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef UseExaHyPE
  tests = exahype2::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

#ifdef UseSWIFT
  tests = swift2::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;
#endif

  if (unitTestsErrors != 0) {
    logError("main()", unitTestsErrors << " unit tests failed. Quit.");
    return EXIT_FAILURE;
  } else {
    logInfo("main()", "All unit tests passed.")
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
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
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  // White- or blacklist assignment checks traces
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox::particles::assignmentchecks",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox::particles::assignmentchecks",
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
      "tarch",
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
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
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
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      0,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

#if Dimensions == 2
  auto DomainOffset = {0.0, 0.0};
  auto DomainSize   = {1.0, 1.0};
#else
  auto DomainOffset = {0.0, 0.0, 0.0};
  auto DomainSize   = {1.0, 1.0, 1.0};
#endif

  peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);

  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

  int returnCode = runTests();

  peano4::shutdown();

  return returnCode;
}
