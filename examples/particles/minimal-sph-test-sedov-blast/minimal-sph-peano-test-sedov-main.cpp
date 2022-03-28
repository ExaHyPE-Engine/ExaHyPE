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
#include "observers/Kick.h"
#include "observers/Kick2.h"
#include "observers/KickCorrector.h"
#include "observers/KickCorrectorExtra.h"
#include "observers/HydroKickExtra.h"
#include "observers/Drift.h"
#include "observers/RearrangeAtStart.h"
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
        // Initial step (e.g., re-arrange the particles into a particular configuration)
        if(i==0){
          examples::particles::observers::RearrangeAtStart rearrangeObserver;
          peano4::parallel::SpacetreeSet::getInstance().traverse(rearrangeObserver);
        }
*/

        /* Name (alias) of the steps that will be used */
        examples::particles::observers::DensityNonSym density;
        examples::particles::observers::HydroPrepareForce prepareForce;
        examples::particles::observers::ForceNonSym forcespart;
        examples::particles::observers::NewtonRaphsonSmoothingLength newtonRaphson;

        for (int iter=0; iter<5; iter++){

          /* Density loop */
          peano4::parallel::SpacetreeSet::getInstance().traverse(density);

          /* "Ghost tasks" */
          /* Prepare force */
          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

          /* Newton Raphson iteration */
          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);

        }

        /* Force loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);

        /*
         * Time integration 
         *
         * */

        /* First Kick from current time-step*/
        examples::particles::observers::Kick kick1part;
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);

        /* Also Kick other properties, e.g., internal energy */
        examples::particles::observers::HydroKickExtra kickExtra;
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);

        /* Drift the particles */
        examples::particles::observers::Drift driftpart;
        peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);

        /*********************************************/
        /* "Option 1: standard KDK" */
        /* Second Force loop */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);
//        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);

        for (int iter=0; iter<5; iter++){

          /* Density loop */
          peano4::parallel::SpacetreeSet::getInstance().traverse(density);

          /* "Ghost tasks" */
          /* Prepare force */
          peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

          /* Newton Raphson iteration */
          peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);

        }

        /* Force loop */
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);

        /* Second Kick from current time-step */
        examples::particles::observers::Kick2 kick2part;
        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
        /*********************************************/

//        /****************************************/
//        /* "Option 2: Predictor-corrector scheme" */
//        /* Not needed according to Matthieu */
//
//        /* Predictor ("kick anyway") step */
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//
//        for(int iter_PC = 0; iter_PC < 5; iter_PC++){
//          /* Second Force loop with predicted velocity */
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

        /* "End time-step" */
        examples::particles::observers::EndTimeStep endTimeStep;
        peano4::parallel::SpacetreeSet::getInstance().traverse(endTimeStep);


      }
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
