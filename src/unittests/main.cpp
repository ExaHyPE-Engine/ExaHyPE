#include "tarch/logging/Log.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/ChromeTraceFileLogger.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"

#include "peano4/peano.h"
#include "peano4/parallel/Node.h"


tarch::logging::Log _log("examples::unittests");


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


int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "examples", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "examples", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));


  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, 0, "tarch", tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, 0, "peano4", tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();

  #ifdef Parallel
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
	  logWarning( "main()", "it is recommended that you run the unit tests with more than one MPI rank, too");
  }
  #endif

  int cores = tarch::multicore::Core::UseDefaultNumberOfThreads;
  if (argc!=2 and tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logWarning( "main()", "usage: ./" + std::string(argv[0]) + " core-count (use default core count=1 now)" );
  }
  else if (argc==2) {
    cores = std::atoi( argv[1] );
  }

  tarch::multicore::Core::getInstance().configure(cores);

  runTests();

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    while ( peano4::parallel::Node::getInstance().continueToRun() ) {
      // Do nothing. This is just to ensure that everything shuts down properly
      // and is properly installed.
    }
  }

  peano4::shutdownParallelEnvironment();

  return 0;
}
