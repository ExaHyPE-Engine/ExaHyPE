#include "minimal-sph-peano-test-sedov-main.h"
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

//
// =================
// inserted manually
// =================
//
#include "observers/CreateGrid.h"
#include "observers/Init.h"
#include "observers/Plot.h"
#include "observers/DensityNonSym.h"
#include "observers/HydroPrepareForce.h"
#include "observers/ForceNonSym.h"
#include "observers/NewtonRaphsonSmoothingLength.h"
#include "observers/Kick1.h"
#include "observers/Kick2.h"
#include "observers/KickCorrector.h"
#include "observers/KickCorrectorExtra.h"
#include "observers/HydroKickExtra.h"
#include "observers/Drift.h"
#include "observers/RearrangeAtStart.h"
#include "observers/ResetHasConverged.h"
#include "observers/InitTimeStep.h"
#include "observers/ImposeBC.h"
#include "observers/EndTimeStep.h"

tarch::logging::Log _log("::");


int main(int argc, char** argv) {
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "peano4",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "tarch",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetDebug,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::particles",
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::particles",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "examples::particles",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  #if Dimensions==2
  auto DomainOffset = {0.0, 0.0};
  auto DomainSize   = {1.0, 1.0};
  #else
  auto DomainOffset = {0.0, 0.0, 0.0};
  auto DomainSize   = {1.0, 1.0, 1.0};
  #endif


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
    }

    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    examples::particles::observers::Init initObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    logInfo( "main()", "added particles" )

    examples::particles::observers::Plot plotObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    logInfo( "main()", "dumped initial condition" )

    for (int i=0; i<500; i++) {
      for (int j=0; j<10; j++) { /* Numbers of time-steps between two snapshots */
        logInfo( "main()", "run timestep" )

        /* 
         * Name the steps that will be used 
         * */

        examples::particles::observers::InitTimeStep initTimeStep;
        examples::particles::observers::DensityNonSym density;
        examples::particles::observers::HydroPrepareForce prepareForce;
        examples::particles::observers::ForceNonSym forcespart;
        examples::particles::observers::NewtonRaphsonSmoothingLength newtonRaphson;
        examples::particles::observers::ResetHasConverged ResetHasConvergedNR;
        examples::particles::observers::Kick1 kick1part;
        examples::particles::observers::Kick2 kick2part;
        examples::particles::observers::HydroKickExtra kickExtra;
        examples::particles::observers::Drift driftpart;
        examples::particles::observers::ImposeBC ImposeBCpart;
        examples::particles::observers::EndTimeStep endTimeStep;

        /* "Initialize time-step" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);

/*
        // Initial step (e.g., re-arrange the particles into a particular configuration)
        if(i==0){
          examples::particles::observers::RearrangeAtStart rearrangeObserver;
          peano4::parallel::SpacetreeSet::getInstance().traverse(rearrangeObserver);
        }
*/

//        if(i==0){ /* For first time-step, we need to compute dotted-quantities here */
//          for (int iter=0; iter<10; iter++){
//            /* Density loop */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//
//            /* Prepare force */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//
//            /* Newton Raphson iteration */
//            peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);
//
//          }
//          /* Density loop */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//
//          /* Prepare force */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//
//          /* Udot */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
//        }
//
//        /* First Kick from current time-step*/
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
//
//        /* Also Kick other properties, e.g., internal energy */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//
//        /* Drift the particles. Also predict v,u,h,rho */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);

        for (int iter=0; iter<2; iter++){
          /* Density loop */
          peano4::parallel::SpacetreeSet::getInstance().traverse(density);

          /* "Ghost tasks" */
          /* Prepare force */
          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

          /* Newton Raphson iteration */
          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);

        }
        /* Density loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(density);

        /* Prepare force */
        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

        /* Force loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);

        /*
         * Time integration 
         *
         * */

        /* First Kick from current time-step*/
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);

        /* Also Kick other properties, e.g., internal energy */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);

        /* Drift the particles */
        peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);

        /* Reset NR convergence so we can update again */
        peano4::parallel::SpacetreeSet::getInstance().traverse(ResetHasConvergedNR);

//        /*********************************************/
//        /* "Option 1: standard KDK" */
//
//        for (int iter=0; iter<1; iter++){
//
//          /* Density loop */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//
//          /* "Ghost tasks" */
//          /* Prepare force */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//
//          /* Newton Raphson iteration */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);
//
//        }
//
//        /* Force loop */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);

        /* Second Kick from current time-step. kick2part also reset predictions */
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
        /*********************************************/

//        /****************************************/
//        /* "Option 2: Predictor-corrector scheme" */
//        /* Not needed according to Matthieu */
//
//        /* Predictor step ("kick anyway") */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//
////        peano4::parallel::SpacetreeSet::getInstance().traverse(ResetHasConvergedNR);
//        for(int iter_PC = 0; iter_PC < 3; iter_PC++){
//          /* Second Force loop with predicted velocity */
//          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);
//          peano4::parallel::SpacetreeSet::getInstance().traverse(density);
//          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//          peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
//  
//          /* Corrector step (mini-Kick) current time-step */
//          examples::particles::observers::KickCorrector corrector;
//          peano4::parallel::SpacetreeSet::getInstance().traverse(corrector);
//          examples::particles::observers::KickCorrectorExtra correctorExtra;
//          peano4::parallel::SpacetreeSet::getInstance().traverse(correctorExtra);
//        }
//        /****************************************/

        /* "Impose BC" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(ImposeBCpart);

        /* "End time-step" */
        peano4::parallel::SpacetreeSet::getInstance().traverse(endTimeStep);

      }

      /* Dump a snapshot after the number of time-steps done above */
      logInfo( "main()", "plot" )
      examples::particles::observers::Plot plotTimeStepObserver;
      peano4::parallel::SpacetreeSet::getInstance().traverse(plotTimeStepObserver);
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
