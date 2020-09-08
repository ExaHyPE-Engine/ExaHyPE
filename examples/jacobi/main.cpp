#include "main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/ChromeTraceFileLogger.h"
#include "tarch/tests/TestCaseRegistry.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "observers/DataRepository.h"
#include "observers/PlotSolution.h"
#include "observers/PlotMaterialParameter.h"
#include "observers/CreateGrid.h"
#include "observers/SetupScenario.h"
#include "observers/ComputeResidualWithGeometricOperators.h"
#include "observers/ComputeGlobalResidualAndError.h"
#include "observers/JacobiUpdate.h"


#ifdef FuseSolverSteps
#include "observers/FusedSolverSteps.h"
#endif


tarch::logging::Log _log("::");


int main(int argc, char* argv[]) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  peano4::initParallelEnvironment(&argc,&argv);
  peano4::initSharedMemoryEnvironment();
  peano4::fillLookupTables();

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::jacobi",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::jacobi",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::jacobi",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::ChromeTraceFileLogger::getInstance().setOutputFile( "trace" );

  #if PeanoDebug>=2
  tarch::tests::TestCaseRegistry::getInstance().getTestCaseCollection().run();
  int unitTestsErrors = tarch::tests::TestCaseRegistry::getInstance()
                       .getTestCaseCollection()
                       .getNumberOfErrors();

  if (unitTestsErrors != 0) {
    logError("main()", "unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  #endif

  peano4::parallel::SpacetreeSet::getInstance().init(
    #if Dimensions==2
    { 0.0, 0.0},
    { 1.0, 1.0},
/*
    {-1.0, -1.0},
    { 2.0,  2.0},
*/
    #else
    {-1.0, -1.0, -1.0},
    { 2.0,  2.0,  2.0},
    #endif
    0
  );


  if (argc>1) {
    examples::jacobi::actions::SetupScenario::Theta = std::atof( argv[1] );
  }
  else {
    logWarning( "main()", "use dummy value for theta " );
  }
  logInfo( "main()", "theta=" << examples::jacobi::actions::SetupScenario::Theta  );

  int coreCount = 1;
  if (argc==3) {
    #ifdef SharedMemoryParallelisation
    coreCount = std::atof( argv[2] );
    logInfo( "main()", "use " << coreCount << " core(s)" );
    #else
    logWarning( "main()", "you should not set the thread count if you translate without multicore support" );
    #endif
  }


  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    do {
      examples::jacobi::observers::CreateGrid  observer;
      peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    }
    while (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<2);

    {
      examples::jacobi::observers::SetupScenario  setupScenario;
      peano4::parallel::SpacetreeSet::getInstance().traverse(setupScenario);

      examples::jacobi::observers::PlotMaterialParameter  plotMaterialParameter;
      peano4::parallel::SpacetreeSet::getInstance().traverse(plotMaterialParameter);
    }

    if (coreCount>1) {
      int cellsPerCore = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() / coreCount;
      logInfo( "main()", "should host " << cellsPerCore << " cells per core" );

      for (int thread=1; thread<coreCount; thread++) {
        if ( not peano4::parallel::SpacetreeSet::getInstance().split(0,cellsPerCore,0)) {
          logWarning( "runParallel(...)", "failed to assign thread " << thread << " " << cellsPerCore << " cell(s)" );
        }
      }
    }


    const int MaxIterations = 40;

    for (int i=0; i<MaxIterations; i++)
    {
      #ifdef FuseSolverSteps
      examples::jacobi::observers::FusedSolverSteps observer;
      peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
      #else
      examples::jacobi::observers::ComputeResidualWithGeometricOperators  computeResidual;
      peano4::parallel::SpacetreeSet::getInstance().traverse(computeResidual);
      examples::jacobi::observers::JacobiUpdate  jacobiUpdate;
      peano4::parallel::SpacetreeSet::getInstance().traverse(jacobiUpdate);
      examples::jacobi::observers::ComputeGlobalResidualAndError computeGlobalResidualAndError;
      peano4::parallel::SpacetreeSet::getInstance().traverse(computeGlobalResidualAndError);
      #endif
    }

    {
      examples::jacobi::observers::PlotSolution  observer;
      peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    }
  }
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      logDebug( "runParallel(...)", "trigger a new sweep with step " << peano4::parallel::Node::getInstance().getCurrentProgramStep() );
    }
  }

  peano4::shutdownSharedMemoryEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
