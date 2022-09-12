#include "minimal-sph-peano-test-noh-problem-main.h"
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

    int gridConstructionSteps = 0;
    while (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<5) {
      logInfo( "main()", "run a grid construction step and insert particles" );
      examples::particles::observers::CreateGrid createObserver;
      peano4::parallel::SpacetreeSet::getInstance().traverse(createObserver);
      gridConstructionSteps++;
      loadBalancer.finishStep();
    }

    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    examples::particles::observers::Init initObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "added particles" )

    examples::particles::observers::Plot plotObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "dumped initial condition" )


    /* Main loop */
    for (int i=0; i<500; i++) {
      for (int j=0; j<10; j++) { /* Numbers of time-steps between two output snapshots */
        logInfo( "main()", "run timestep" )

        /* 
         * Name the steps that will be used 
         * */

        examples::particles::observers::InitTimeStep initTimeStep;

/*
        examples::particles::observers::DensityNonSym density;
        examples::particles::observers::HydroPrepareForce prepareForce;
        examples::particles::observers::NewtonRaphsonSmoothingLength newtonRaphson;
*/
        examples::particles::observers::ForceNonSym forcespart;
        examples::particles::observers::ComputeDensityAndUpdateSmoothingLength computeDensityAndUpdateSmoothingLength;


        examples::particles::observers::ResetHasConverged ResetHasConvergedNR;
        examples::particles::observers::Drift driftpart;
        examples::particles::observers::Kick1 kick1part;
        examples::particles::observers::Kick2 kick2part;
        examples::particles::observers::HydroKickExtra kickExtra;
        examples::particles::observers::PredictHydro predictHydroPart;
        examples::particles::observers::ImposeBC ImposeBCpart;
        examples::particles::observers::EndTimeStep endTimeStep;

        /* "Initialize time-step" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* For first time-step, we need to pre-compute some quantities before drifting */
        /* This is because we must start the simulation with a "K1" step */
        if(i==0 and j==0){ 
          for (int iter=0; iter<5; iter++){
            peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
            examples::particles::vertexdata::ParticleSet::finishedTraversal();

            /* "Hydro loop" */
            /* Density loop */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//            examples::particles::vertexdata::ParticleSet::finishedTraversal();
            /* "Ghost tasks" */
            /* Prepare force */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//            examples::particles::vertexdata::ParticleSet::finishedTraversal();
            /* Newton Raphson iteration */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);
//            examples::particles::vertexdata::ParticleSet::finishedTraversal();
          }

        /* Force loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* Reset NR convergence so we can update again */
        peano4::parallel::SpacetreeSet::getInstance().traverse(ResetHasConvergedNR);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Kick1" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part); // use Kick2 to pass values to V,U arrays
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
        } /* Finish the "0th" time-step */

        /* Now the time-step in the actual "D-K2-K1" form begins */
        /* "Drift" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
//        /* Also predicts v, u, h, ... */
//        /* Only relevant for local time-stepping */
        peano4::parallel::SpacetreeSet::getInstance().traverse(predictHydroPart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        for (int iter=0; iter<3; iter++){
          peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
          examples::particles::vertexdata::ParticleSet::finishedTraversal();

          /* "Hydro loop" */
          /* Density loop */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//          examples::particles::vertexdata::ParticleSet::finishedTraversal();
          /* "Ghost tasks" */
          /* Prepare force */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//          examples::particles::vertexdata::ParticleSet::finishedTraversal();
          /* Newton Raphson iteration */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);
//          examples::particles::vertexdata::ParticleSet::finishedTraversal();
        }

        /* Force loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Kick2". kick2part also reset predictions for next Drift */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Kick1" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();
        /* Also Kick other properties, e.g., internal energy */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "Impose BC" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(ImposeBCpart);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

        /* "End time-step" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(endTimeStep);
        examples::particles::vertexdata::ParticleSet::finishedTraversal();

      }

      /* Dump a snapshot after the number of time-steps done above */
      logInfo( "main()", "plot" );
      examples::particles::observers::Plot plotTimeStepObserver;
      peano4::parallel::SpacetreeSet::getInstance().traverse(plotTimeStepObserver);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();
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
