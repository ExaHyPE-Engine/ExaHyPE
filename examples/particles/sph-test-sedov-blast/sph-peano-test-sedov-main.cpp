#include "sph-peano-test-sedov-main.h"
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
#include "observers/HydroPrepareGradient.h"
#include "observers/HydroResetGradient.h"
#include "observers/GradientNonSym.h"
#include "observers/HydroPrepareForce.h"
#include "observers/ForceNonSym.h"
#include "observers/NewtonRaphsonSmoothingLength.h"
#include "observers/Kick.h"
#include "observers/HydroKickExtra.h"
#include "observers/Drift.h"
#include "observers/RearrangeAtStart.h"

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
      for (int j=0; j<10; j++) {
        logInfo( "main()", "run timestep" )

/*
        // Debug Step (if 1D setup is needed)
        if(i==0){
          examples::particles::observers::RearrangeAtStart rearrangeObserver;
          peano4::parallel::SpacetreeSet::getInstance().traverse(rearrangeObserver);
        }
*/

        /* Synchro step (as in N-body -- might not be accurate here) */
        if(i>0){ // Do not synchronize at the IC
          examples::particles::observers::Kick synchroObserver;                // Synchronization step (last kick from previous time-step)
          peano4::parallel::SpacetreeSet::getInstance().traverse(synchroObserver);
        }

        /* Density loop */
        examples::particles::observers::DensityNonSym densityObserver;
        peano4::parallel::SpacetreeSet::getInstance().traverse(densityObserver);

        examples::particles::observers::HydroPrepareGradient prepareGradient;
        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareGradient);

        examples::particles::observers::HydroResetGradient ResetGradient;
        peano4::parallel::SpacetreeSet::getInstance().traverse(ResetGradient);

        /* Gradient loop */
        examples::particles::observers::GradientNonSym Gradient;
        peano4::parallel::SpacetreeSet::getInstance().traverse(Gradient);

        examples::particles::observers::NewtonRaphsonSmoothingLength newtonRaphson;
        peano4::parallel::SpacetreeSet::getInstance().traverse(newtonRaphson);

        examples::particles::observers::HydroPrepareForce prepareForce;
        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

        /* Force loop */
        examples::particles::observers::ForceNonSym forcesObserver;
        peano4::parallel::SpacetreeSet::getInstance().traverse(forcesObserver);

        /* Time integration */
        examples::particles::observers::Kick kickObserver;                     // First Kick from current time-step
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickObserver);

        examples::particles::observers::HydroKickExtra kickExtra;              // Evolve internal energy
        peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);

        examples::particles::observers::Drift driftObserver;                   // Drift from current time-step
        peano4::parallel::SpacetreeSet::getInstance().traverse(driftObserver);

//        examples::particles::observers::Kick kickObserver;                     // Second Kick from current time-step
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kickObserver);

        /* Get new acceleration -- Eq 68 of Phantom paper */
//        examples::particles::observers::HydroPrepareForce prepareForce;
//        peano4::parallel::SpacetreeSet::getInstance().traverse(prepareForce);

//        examples::particles::observers::ForceNonSym forcesObserver;
//      peano4::parallel::SpacetreeSet::getInstance().traverse(forcesObserver);

        /* Final, third kick -- Eq 69 of Phantom paper */
//        examples::particles::observers::Kick2 kick2;                     
//        peano4::parallel::SpacetreeSet::getInstance().traverse(kick2);

     
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
