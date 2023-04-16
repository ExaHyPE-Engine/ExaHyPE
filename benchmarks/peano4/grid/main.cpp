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


tarch::logging::Log _log("examples::grid");


std::bitset<Dimensions> periodicBC = 0;

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
    tarch::mpi::Rank::abort( -2 );
  }
}



void runSerial() {
  examples::grid::MyObserver emptyObserver;

  peano4::initSingletons(
#if Dimensions==2
    {0.0, 0.0},
    {1.0, 1.0},
#else
    {0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0},
#endif
    periodicBC
  );
  
  
  // You wouldn't do this usually. We always work against the set.
  peano4::grid::Spacetree         spacetree(
#if Dimensions==2
    {0.0, 0.0},
    {1.0, 1.0}
#else
    {0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0}
#endif
  );
  

  for (int i=0; i<2*examples::grid::MyObserver::GridRefinementIterations + examples::grid::MyObserver::StationaryIterations; i++) {
	tarch::logging::CommandLineLogger::getInstance().closeOutputStreamAndReopenNewOne();

    #if PeanoDebug>0
	  emptyObserver.beginTraversalOnRank(false);
    #endif
    spacetree.traverse( emptyObserver );
    #if PeanoDebug>0
    emptyObserver.endTraversalOnRank(false);
    #endif

    logInfo( "runSerial(...)", "grid statistics = " << spacetree.getGridStatistics().toString() );
  }


  peano4::grid::TraversalVTKPlotter plotterObserver( "grid-serial" );
  spacetree.traverse( plotterObserver );

  peano4::shutdownSingletons();
}


void updateDomainDecomposition() {
  static int phase = 0;

  int targetRank = (phase+1) % tarch::mpi::Rank::getInstance().getNumberOfRanks();

  if (phase==0) {
    const int spacetreeOfInterest = 0;

    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if (
        peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD
        and
        peano4::parallel::SpacetreeSet::getInstance().split(0,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3,targetRank)
        and
        peano4::parallel::SpacetreeSet::getInstance().split(0,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3,targetRank)
      ) {
        logInfo( "updateDomainDecomposition()", "split rank 0 into 1 and 2 -> success" );
        phase++;
      }
    }
    else phase++;
  }
  else if (
    phase==1
    and
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD*2
  ) {
    const int spacetreeOfInterest = 1;

    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(1,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3/3,targetRank) ) {
        logInfo( "updateDomainDecomposition()", "split rank 1 -> failed" );
        phase++;
      }
      else {
        logInfo( "updateDomainDecomposition()", "split rank 1 -> success" );
      }
    }
    else phase++;
  }
  else if (
    phase==2
    and
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD*4
  ) {
    const int spacetreeOfInterest = periodicBC==0 ? 1 : 0;
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(
        spacetreeOfInterest,
        peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3/3/3,targetRank)
      ) {
        phase++;
      }
    }
    else phase++;
  }
  else if (
    phase==3
    and
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD*6
  ) {
    const int spacetreeOfInterest = 2;
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(2,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3/3/3/3,targetRank) ) {
        phase++;
      }
    }
    else phase++;
  }
  else if (
    phase==4
    and
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD*8
  ) {
    const int spacetreeOfInterest = 2;
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(spacetreeOfInterest,peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()/3/3/3/3/3,targetRank) ) {
        phase++;
      }
    }
    else phase++;
  }
  else if (
    phase==5
    and
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells()>=ThreePowerD*10
  ) {
    const int spacetreeOfInterest = 1;
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree(spacetreeOfInterest) ) {
      if ( not peano4::parallel::SpacetreeSet::getInstance().split(1,10,targetRank) ) {
        phase++;
      }
    }
    else phase++;
  }
}


void runParallel() {
  peano4::initSingletons(
#if Dimensions==2
    {0.0, 0.0},
    {1.0, 1.0},
#else
    {0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0},
#endif
    periodicBC
  );

  examples::grid::MyObserver emptyObserver;

  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(14);
    for (int i=0; i<2*examples::grid::MyObserver::GridRefinementIterations + examples::grid::MyObserver::StationaryIterations; i++) {
      tarch::logging::CommandLineLogger::getInstance().closeOutputStreamAndReopenNewOne();
      #if PeanoDebug>0
      emptyObserver.beginTraversalOnRank(true);
      #endif
      peano4::parallel::SpacetreeSet::getInstance().traverse( emptyObserver );
      #if PeanoDebug>0
      emptyObserver.endTraversalOnRank(true);
      #endif

      logInfo( "runParallel(...)", "grid statistics = " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().toString() );

      updateDomainDecomposition();
    }

    logInfo( "runParallel(...)", "terminated successfully" );
  }
  else { // not the global master
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      assertionEquals( peano4::parallel::Node::getInstance().getCurrentProgramStep(), 14 );

      tarch::logging::CommandLineLogger::getInstance().closeOutputStreamAndReopenNewOne();

      #if PeanoDebug>0
      emptyObserver.beginTraversalOnRank(true);
      #endif
      peano4::parallel::SpacetreeSet::getInstance().traverse(emptyObserver);
      #if PeanoDebug>0
      emptyObserver.endTraversalOnRank(true);
      #endif
    }
  }

  peano4::shutdownSingletons();
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
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, tarch::logging::LogFilter::FilterListEntry::AnyRank, "tarch", false
  ));
  tarch::logging::CommandLineLogger::getInstance().setOutputFile( "trace.txt" );

  tarch::multicore::Core::getInstance().configure(4,2,1);

  runTests();

  if (argc==2) {
    periodicBC = std::atoi( argv[1] );
  }
  else {
    srand( time(NULL) );
    periodicBC = 0;
    for (int i=0; i<Dimensions; i++) {
      periodicBC[i] = rand() < RAND_MAX / 5;
    }
    logInfo( "main(...)", "use a random periodic boundary condition choice: " << periodicBC );
  }

  #if defined(Parallel) or defined(SharedMemoryParallelisation)
  runParallel();
  #else
  runSerial();
  #endif

  peano4::shutdownParallelEnvironment();

  return 0;
}
