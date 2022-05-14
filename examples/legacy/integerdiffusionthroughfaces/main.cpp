#include "MyObserver.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/parallel/Node.h"


tarch::logging::Log _log("examples::integerdiffusionthroughfaces");




#include "peano4/UnitTests.h"
#include "tarch/UnitTests.h"


void runTests() {
  int unitTestsErrors = 0;
  tarch::tests::TestCase* tests = nullptr;

  tests = tarch::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = peano4::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  if (unitTestsErrors != 0) {
    logError("main()", "unit tests failed. Quit.");
    exit(-2);
  }
}



void runExample() {
  peano4::parallel::SpacetreeSet::getInstance().init(
#if Dimensions==2
    {0.0, 0.0},
    {1.0, 1.0}
#else
    {0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0}
#endif
  );

  examples::integerdiffusionthroughfaces::MyObserver emptyObserver;

  for (int i=0; i<40; i++) {
	tarch::logging::CommandLineLogger::getInstance().closeOutputStreamAndReopenNewOne();
    peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );

    const int SplitEveryKIterations = 5;
    if (i%SplitEveryKIterations==SplitEveryKIterations-1) {
      peano4::parallel::SpacetreeSet::getInstance().split(0,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3,0);
    }

    logInfo( "main(...)", "grid statistics = " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().toString() );
  }

  // @todo Das muss halt auch alles mit Multicore tun

  logInfo( "main(...)", "quit successfully" );
}



int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", true
  ));
  tarch::logging::CommandLineLogger::getInstance().setOutputFile( "trace.txt" );

  tarch::multicore::Core::getInstance().configure(4,2,1);

  runTests();
  runExample();

  peano4::shutdownParallelEnvironment();

  return 0;
}
