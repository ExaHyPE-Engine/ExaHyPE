#include "test_balsara_switch_1D-main.h"
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
#include "observers/DumpBalsara.h"

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

    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    examples::particles::observers::Init initObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    logInfo( "main()", "added particles" )

    examples::particles::observers::Plot plotObserver;
    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    logInfo( "main()", "dumped initial condition" )
        
    /* Main function */
    /* Just call the iterative density-smoothing length calculation */
    logInfo( "main()", "run timestep" )

    examples::particles::observers::ComputeDensityAndUpdateSmoothingLength computeDensityAndUpdateSmoothingLength;
    examples::particles::observers::DumpBalsara dumpBalsara;
    examples::particles::observers::Plot plotTimeStepObserver;

    for (int iter=0; iter<1; iter++){
      logInfo( "main()", "Iteration number:" << iter );
      peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

      peano4::parallel::SpacetreeSet::getInstance().traverse(dumpBalsara);
      examples::particles::vertexdata::ParticleSet::finishedTraversal();

    /* Dump a snapshot */
    logInfo( "main()", "plot" );
    peano4::parallel::SpacetreeSet::getInstance().traverse(plotTimeStepObserver);
    examples::particles::vertexdata::ParticleSet::finishedTraversal();
    }

    logInfo( "main()", "test terminated successfully" )

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
