#include "minimal-sph-peano-test-sod-shock-main.h"
#include "Constants.h"
#include "kernel_hydro.h"
#include "equation_of_state.h"

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
#include "observers/ComputeDensityAndUpdateSmoothingLength.h"
#include "observers/ForceNonSym.h"
#include "observers/Drift.h"
#include "observers/Kick1.h"
#include "observers/Kick2.h"
#include "observers/PredictHydro.h"
#include "observers/KickCorrector.h"
#include "observers/KickCorrectorExtra.h"
#include "observers/HydroKickExtra.h"
#include "observers/ResetHasConverged.h"
#include "observers/InitTimeStep.h"
#include "observers/ImposeBC.h"
#include "observers/EndTimeStep.h"

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

    /**
     * ==================================================
     * SPH simulation steps for minimal-SPH hydro scheme
     * ==================================================
     *
     * Now comes the core algorithmic steps of the SPH simulation.
     *
     */

    /**
     * Name the steps that will be used in the simulation.
     * Each of these steps consist in a sequence of touch particle first/last
     * and particle-particle compute kernels. Their implementation can be found
     * in the sph toolbox python library.
     *
     */

    examples::particles::observers::ComputeDensityAndUpdateSmoothingLength computeDensityAndUpdateSmoothingLength;
    examples::particles::observers::CreateGrid         createObserver;
    examples::particles::observers::Init               initObserver;
    examples::particles::observers::InitTimeStep       initTimeStep;
    examples::particles::observers::ForceNonSym        forcespart;
    examples::particles::observers::ResetHasConverged  resetHasConvergedNR;
    examples::particles::observers::Drift              driftpart;
    examples::particles::observers::Kick1              kick1part;
    examples::particles::observers::Kick2              kick2part;
    examples::particles::observers::HydroKickExtra     kickExtra;
    examples::particles::observers::PredictHydro       predictHydroPart;
    examples::particles::observers::ImposeBC           imposeBCpart;
    examples::particles::observers::EndTimeStep        endTimeStep;
    examples::particles::observers::Plot               plotObserver;

    int gridConstructionSteps = 0;
    while (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<5) {
      logInfo( "main()", "run a grid construction step and insert particles" );
      peano4::parallel::SpacetreeSet::getInstance().traverse(createObserver);
      gridConstructionSteps++;
      loadBalancer.finishStep();
    }

    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "added particles" )

    /* Run any action to re-assign particles to vertices */
    peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();

    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "dumped initial condition" )


    /** 
     * Main time-step loop.
     */

    const int TotalNumberOfTimeSteps = 2000;

    // Frequency for output snapshots
    const int DeltaSnapshots = 10;

    for (int i=1; i<=TotalNumberOfTimeSteps; i++) {

      logInfo( "main()", "run timestep " << i )

      /* "Initialize time-step" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* For first time-step, we need to pre-compute some quantities before drifting */
      /* This is because we must start the simulation with a "K1" step */
      if (i == 0){ 
        for (int iter=0; iter<5; iter++){
          peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
          examples::particles::vertexdata::ParticleSet::finishedTraversal();
        }

      /* Reset NR convergence so we can update again */
      peano4::parallel::SpacetreeSet::getInstance().traverse(resetHasConvergedNR);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* Force loop */
      /* Update vDot, uDot */
      peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* "Kick1" */
      /* Update V_full and U_full. Pass them to U and V arrays */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
//      examples::particles::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//      examples::particles::vertexdata::ParticleSet::finishedTraversal();
      } /* Finish the "0th" time-step */

      /* Now the time-step in the actual "D-K2-K1" form begins */
      /* "Drift" */
      /* Update position */
      peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
//      /* Predict  V, U, h, rho */
      peano4::parallel::SpacetreeSet::getInstance().traverse(predictHydroPart);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      for (int iter=0; iter<3; iter++){
        peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
      }

      /* Force loop */
      peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* "Kick2"*/
      /* Update V_full and U_full. Pass them to U and V arrays */
      /* Now all values at n+1 are known */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* Move into next time-step with replacement n+1 -> n */
      /* "Kick1" */
      /* Update V_full and U_full */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* "Impose BC" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(imposeBCpart);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* "End time-step" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(endTimeStep);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      /* Do we need to dump the snapshot? */
      if (i % DeltaSnapshots == 0){
        logInfo( "main()", "plot timestep " << i );
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
