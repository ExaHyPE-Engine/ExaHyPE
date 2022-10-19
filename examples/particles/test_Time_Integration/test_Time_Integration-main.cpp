#include "test_Time_Integration-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"

#include "peano4/UnitTests.h"
#include "tarch/UnitTests.h"

// @todo docu
#include "toolbox/loadbalancing/RecursiveSubdivision.h"
#include "tarch/logging/LogFilterFileReader.h"


//
// =================
// inserted manually
// =================
//
#include "observers/CreateGrid.h"
#include "observers/Init.h"
#include "observers/Plot.h"
#include "observers/ForceGravity.h"
#include "observers/Kick1.h"
#include "observers/Kick2.h"
#include "observers/Drift.h"
#include "observers/InitTimeStep.h"

tarch::logging::Log _log("::");


int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;


  #if Dimensions==2
  auto DomainOffset = {0.0, 0.0};
  auto DomainSize   = {1.0, 1.0};
  #else
  auto DomainOffset = {0.0, 0.0, 0.0};
  auto DomainSize   = {1.0, 1.0, 1.0};
  #endif

  std::string logFilterFile = "swift2.log-filter";
  if ( not tarch::logging::LogFilterFileReader::parsePlainTextFile( logFilterFile ) ) {
    logWarning( "main()", "no swift2.log-filter file found or file has been corrupted. Use default logging configuration" );
  }


  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();
  examples::particles::repositories::DataRepository::initDatatypes();
  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    0
  );

  #if PeanoDebug>=2
  tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {
    logError("main()", "Peano4 core unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete peanoCoreTests;

  tarch::tests::TestCase* peanoTarchTests = tarch::getUnitTests();
  peanoTarchTests->run();
  if (peanoTarchTests->getNumberOfErrors() != 0) {
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete peanoTarchTests;

  #endif

  /**
   * @todo Make available via Python
   */
  toolbox::loadbalancing::RecursiveSubdivision              loadBalancer;

  #if defined(SharedOMP)
  #pragma omp parallel
  {
  #pragma omp master
  {
  #endif
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {

    //
    // =================
    // inserted manually
    // =================
    //

    /* 
     * Name the steps that will be used 
     */
    examples::particles::observers::InitTimeStep initTimeStep;
    examples::particles::observers::ForceGravity forcespart;
    examples::particles::observers::Kick1 kick1part;
    examples::particles::observers::Kick2 kick2part;
    examples::particles::observers::Drift driftpart;
    examples::particles::observers::CreateGrid createObserver;
    examples::particles::observers::Init initObserver;
    examples::particles::observers::Plot plotObserver;

    int gridConstructionSteps = 0;
    while (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<5) {
      logInfo( "main()", "run a grid construction step and insert particles" );
      peano4::parallel::SpacetreeSet::getInstance().traverse(createObserver);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
      gridConstructionSteps++;
      loadBalancer.finishStep();
    }

    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "added particles" )

    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "dumped initial condition" )
        
    /* Main loop */

    const int TimeStepsInBetweenTwoSnapshots = std::max(
      10,
      static_cast<int>( std::round( 1.0/examples::particles::GLOBAL_TIME_STEP_SIZE )) / 20
    );
    logInfo( "main()", "steps between two snapshots=" << TimeStepsInBetweenTwoSnapshots );

    for (int i=0; i<1000; i++) {
      for (int j=0; j<TimeStepsInBetweenTwoSnapshots; j++) 
      { 
        logInfo( "main()", "run timestep: " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().toString() );

        /* "Initialize time-step" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* For first time-step, we need to pre-compute some quantities before drifting */
        /* This is because we must start the simulation with a "K1" step */
        if (i==0 and j==0) 
        { 
          /* Force Calculation */
          peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
          examples::particles::vertexdata::ParticleSet::finishedTraversal();

          /* "Kick1" */
          peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
          examples::particles::vertexdata::ParticleSet::finishedTraversal();
        } /* Finish the "0th" time-step */

        /* Now the time-step in the actual "D-K2-K1" form begins */
        /* "Drift". Drift also predicts v, u, ... */
        peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* Force Calculation */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Kick2". kick2part also reset predictions for next Drift */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Kick1" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

      }

      if (examples::particles::PLOT) {
        /* Dump a snapshot after the number of time-steps done above */
        logInfo( "main()", "plot" );
        peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
      }
    }

    logInfo( "main()", "terminated successfully" )

    //
    // =================
    // End of manually inserted block
    // =================
    //

}
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      logDebug( "runParallel(...)", "trigger a new sweep with step " << peano4::parallel::Node::getInstance().getCurrentProgramStep() );
    }
  }
  #if defined(SharedOMP)
  }
  }
  #endif

  peano4::shutdownSingletons();
  examples::particles::repositories::DataRepository::shutdownDatatypes();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
