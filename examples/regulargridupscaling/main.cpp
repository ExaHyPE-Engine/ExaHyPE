#include "MyObserver.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/ChromeTraceFileLogger.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/parallel/Node.h"


tarch::logging::Log _log("examples::regulargridupscaling");


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




void runParallel(double h, int flopsPerCell) {
  logTraceInWith1Argument( "runParallel", h );
  peano4::parallel::SpacetreeSet::getInstance().init(
    #if Dimensions==2
    {0.0, 0.0},
    {1.0, 1.0},
    #else
    {0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0},
    #endif
    0
  );

  examples::regulargridupscaling::MyObserver emptyObserver(examples::regulargridupscaling::MyObserver::RanksObserverTemplate,h,flopsPerCell);

  int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    logInfo( "runParallel(...)", "create initial grid (step #1)" );
    int numberOfGridConstructionSteps = 0;
    while (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <
      tarch::mpi::Rank::getInstance().getNumberOfRanks() * ThreePowerD
      and
      numberOfGridConstructionSteps<20
    ) {
      peano4::parallel::Node::getInstance().setNextProgramStep(1);
      peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );
      numberOfGridConstructionSteps++; 
    }
    logInfo( "runParallel(...)", "grid statistics = " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().toString() );

    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <
      tarch::mpi::Rank::getInstance().getNumberOfRanks() 
    ) {
      logError( "runParallel(...)", "not enough cells to keep " << tarch::mpi::Rank::getInstance().getNumberOfRanks() << " busy" );
      exit(-1);
    }

    int numberOfCellsPerRank = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / tarch::mpi::Rank::getInstance().getNumberOfRanks();
    logInfo( "runParallel(...)", "trigger split of initial grid among ranks with " << numberOfCellsPerRank << " cells per rank" );
    for (int rank=1; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(0,numberOfCellsPerRank,rank)) {
        logWarning( "runParallel(...)", "failed to assign rank " << rank << " " << numberOfCellsPerRank << " cell(s)" );
      }
    }

    const int MaxNumberOfConstructionSteps = static_cast<int>(std::round(  std::log(1.0 / h)/std::log(3.0)+1  )) - 1;
    assertion1(MaxNumberOfConstructionSteps>=0, MaxNumberOfConstructionSteps);
    logInfo( "runParallel(...)", "commit split and give ranks " << MaxNumberOfConstructionSteps << " iterations to 'recover' (step #2)" );
    for (int i=0; i<MaxNumberOfConstructionSteps; i++) {
      peano4::parallel::Node::getInstance().setNextProgramStep(2);
      peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );
    }

    int numberOfCellsPerThread = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / numberOfThreads;
    logInfo( "runParallel(...)", "trigger split of master rank into threads with " << numberOfCellsPerThread  << " cells per thread (total: " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() << ")");
    for (int thread=1; thread<numberOfThreads; thread++) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(0,numberOfCellsPerThread,0)) {
        logWarning( "runParallel(...)", "failed to assign thread " << thread << " " << numberOfCellsPerThread << " cell(s)" );
      }
    }

    logInfo( "runParallel(...)", "run one step committing split and telling other ranks to split as well (step #3)" );
    peano4::parallel::Node::getInstance().setNextProgramStep(3);
    peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );

    logInfo( "runParallel(...)", "commit splits into threads and give ranks time to 'recover' (step #4)" );
    for (int i=0; i<3; i++) {
      peano4::parallel::Node::getInstance().setNextProgramStep(4);
      peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );
    }

    logInfo( "runParallel(...)", "start parallel traversals (step #5)" );
    logInfo( "runParallel(...)", "grid statistics = " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().toString() );

    #if PeanoDebug>=2
    const int Iterations=2;
    #else
    const int Iterations=20;
    #endif

    for (int i=0; i<Iterations; i++) {
      peano4::parallel::Node::getInstance().setNextProgramStep(5);
      peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );
    }
    logInfo( "runParallel(...)", "terminated successfully" );
  }
  else { // not the global master
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      logDebug( "runParallel(...)", "trigger a new sweep with step " << peano4::parallel::Node::getInstance().getCurrentProgramStep() );
      if (
        peano4::parallel::Node::getInstance().getCurrentProgramStep()==2
      or
        peano4::parallel::Node::getInstance().getCurrentProgramStep()==4
      or
        peano4::parallel::Node::getInstance().getCurrentProgramStep()==5
      ) {
        peano4::parallel::SpacetreeSet::getInstance().traverse(emptyObserver);
      }
      else if (peano4::parallel::Node::getInstance().getCurrentProgramStep()==3) {
        assertionEquals( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1);
        const int localTree = *(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin());
        int numberOfCellsPerThread = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / tarch::multicore::Core::getInstance().getNumberOfThreads();
        logInfo( "runParallel(...)", "local unrefined cells = " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells());
        logInfo( "runParallel(...)", "trigger split of master rank into threads with " << numberOfCellsPerThread << " cells per thread");
        for (int thread=1; thread<numberOfThreads; thread++) {
          if ( not peano4::parallel::SpacetreeSet::getInstance().split(localTree,numberOfCellsPerThread,tarch::mpi::Rank::getInstance().getRank())) {
            logWarning( "runParallel(...)", "failed to assign thread " << thread << " " << numberOfCellsPerThread << " cell(s)" );
          }
        }
        peano4::parallel::SpacetreeSet::getInstance().traverse(emptyObserver);
      }
    }
  }
  logTraceOut( "runParallel" );
}


int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", false
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", false
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "peano4", false
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", true
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", false
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", true
  ));

  //tarch::logging::CommandLineLogger::getInstance().setOutputFile( "trace.txt" );
  tarch::logging::ChromeTraceFileLogger::getInstance().setOutputFile( "p4.tracing" );

  runTests();

  if (argc<3 or argc>6) {
  	logError( "main(...)", "Usage: ./executable mesh-width flops-per-cell [core-count] [spawn-frequency] [integration-points]");
	  return 1;
  }

  double meshWidth    = std::atof( argv[1] );
  int    flopsPerCell = std::atoi( argv[2] );
  if (meshWidth<=0) {
    logError( "main(...)", "Usage: ./executable mesh-width");
    logError( "main(...)", "  mesh-width has to be a positive value");
    return 2;
  }
  if (meshWidth>=1.0) {
    logError( "main(...)", "Usage: ./executable mesh-width");
    logError( "main(...)", "  mesh-width has to be smaller than one");
    return 2;
  }

  if (argc>=4) {
    int cores = std::atoi( argv[3] );
    tarch::multicore::Core::getInstance().configure(cores);
  }

  if (argc>=5) {
    examples::regulargridupscaling::MyObserver::FractionOfCellsYieldingIntegrationTask = std::atof( argv[4] );
  }
  if (argc>=6) {
    examples::regulargridupscaling::MyObserver::IntegrationAccuracy = std::atoi( argv[5] );
  }

  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int numberOfCores = tarch::multicore::Core::getInstance().getNumberOfThreads();
  logInfo( "main(...)", "run on " << numberOfRanks << " ranks with " << numberOfCores << " thread(s) each" );

  runParallel(meshWidth,flopsPerCell);

  peano4::shutdownParallelEnvironment();

  return 0;
}
