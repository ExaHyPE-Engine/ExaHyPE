#include "{MAIN_NAME}.h"
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

#ifdef UseFEExcept
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
#endif

/* Headers for SPH simulation steps (minimal SPH) */
// @TODO add SPH directive, i.e 
// #ifdef SPH
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
// #endif SPH

tarch::logging::Log _log("::");


int main(int argc, char** argv) {{
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

#ifdef UseFEExcept
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

//#ifndef SPH
/*
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
    "{FULL_NAMESPACE}", 
    tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetTrace, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "{FULL_NAMESPACE}", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetInfo, 
    tarch::logging::LogFilter::FilterListEntry::AnyRank, 
    "{FULL_NAMESPACE}", 
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
    tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

*/
//#endif


  #if Dimensions==2
  auto DomainOffset = {{0.0, 0.0}};
  auto DomainSize   = {{1.0, 1.0}};
  #else
  auto DomainOffset = {{0.0, 0.0, 0.0}};
  auto DomainSize   = {{1.0, 1.0, 1.0}};
  #endif

// #ifdef SPH
  std::string logFilterFile = "swift2.log-filter";
  if ( not tarch::logging::LogFilterFileReader::parsePlainTextFile( logFilterFile ) ) {{
    logWarning( "main()", "no swift2.log-filter file found or file has been corrupted. Use default logging configuration" );
  }}
// #endif


  peano4::initParallelEnvironment(&argc,&argv);
  peano4::fillLookupTables();
  {FULL_NAMESPACE}::repositories::DataRepository::initDatatypes();
  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    0
  );
  
  #if PeanoDebug>=2
  tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {{
    logError("main()", "Peano4 core unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }}
  delete peanoCoreTests;

  tarch::tests::TestCase* peanoTarchTests = tarch::getUnitTests();
  peanoTarchTests->run();
  if (peanoTarchTests->getNumberOfErrors() != 0) {{
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }}
  delete peanoTarchTests;
 
  #endif

// #ifdef SPH
  /**
   * @todo Make available via Python
   */
  toolbox::loadbalancing::RecursiveSubdivision              loadBalancer;
// #endif

  #if defined(SharedOMP)
  #pragma omp parallel
  {{
  #pragma omp master 
  {{
  #endif
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {{

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

    {FULL_NAMESPACE}::observers::ComputeDensityAndUpdateSmoothingLength computeDensityAndUpdateSmoothingLength;
    {FULL_NAMESPACE}::observers::CreateGrid         createObserver;
    {FULL_NAMESPACE}::observers::Init               initObserver;
    {FULL_NAMESPACE}::observers::InitTimeStep       initTimeStep;
    {FULL_NAMESPACE}::observers::ForceNonSym        forcespart;
    {FULL_NAMESPACE}::observers::ResetHasConverged  resetHasConvergedNR;
    {FULL_NAMESPACE}::observers::Drift              driftpart;
    {FULL_NAMESPACE}::observers::Kick1              kick1part;
    {FULL_NAMESPACE}::observers::Kick2              kick2part;
    {FULL_NAMESPACE}::observers::HydroKickExtra     kickExtra;
    {FULL_NAMESPACE}::observers::PredictHydro       predictHydroPart;
    {FULL_NAMESPACE}::observers::ImposeBC           imposeBCpart;
    {FULL_NAMESPACE}::observers::EndTimeStep        endTimeStep;
    {FULL_NAMESPACE}::observers::Plot               plotObserver;

    int gridConstructionSteps = 0;
    while
    (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()<5) {{
      logInfo( "main()", "run a grid construction step and insert particles" );
      peano4::parallel::SpacetreeSet::getInstance().traverse(createObserver);
      gridConstructionSteps++;
      loadBalancer.finishStep();
    }}

    logInfo( "main()", "finished grid construction after " << gridConstructionSteps << " steps, start to add particles" )

    peano4::parallel::SpacetreeSet::getInstance().traverse(initObserver);
    {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "added particles" )

    peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
    {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
    logInfo( "main()", "dumped initial condition (timestep 0)" )


    /** 
     * Main time-step loop.
     */

    const int TotalNumberOfTimeSteps = 500;

    // Frequency for output snapshots
    const int DeltaSnapshots = 10;

    for (int i=1; i<TotalNumberOfTimeSteps; i++) {{

      logInfo( "main()", "run timestep " << i )

      /* "Initialize time-step" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(initTimeStep);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* For first time-step, we need to pre-compute some quantities before drifting */
      /* This is because we must start the simulation with a "K1" step */
      if (i == 0){{ 
        for (int iter=0; iter<5; iter++){{
          peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
          {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
        }}

      /* Reset NR convergence so we can update again */
      peano4::parallel::SpacetreeSet::getInstance().traverse(resetHasConvergedNR);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* Force loop */
      /* Update vDot, uDot */
      peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* "Kick1" */
      /* Update V_full and U_full. Pass them to U and V arrays */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
//      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      }} /* Finish the "0th" time-step */

      /* Now the time-step in the actual "D-K2-K1" form begins */
      /* "Drift" */
      /* Update position */
      peano4::parallel::SpacetreeSet::getInstance().traverse(driftpart);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
//      /* Predict  V, U, h, rho */
      peano4::parallel::SpacetreeSet::getInstance().traverse(predictHydroPart);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      for (int iter=0; iter<3; iter++){{
        peano4::parallel::SpacetreeSet::getInstance().traverse(computeDensityAndUpdateSmoothingLength);
        {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      }}

      /* Force loop */
      peano4::parallel::SpacetreeSet::getInstance().traverse(forcespart);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* "Kick2"*/
      /* Update V_full and U_full. Pass them to U and V arrays */
      /* Now all values at n+1 are known */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick2part);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
//      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
//      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* Move into next time-step with replacement n+1 -> n */
      /* "Kick1" */
      /* Update V_full and U_full */
      peano4::parallel::SpacetreeSet::getInstance().traverse(kick1part);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      peano4::parallel::SpacetreeSet::getInstance().traverse(kickExtra);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* "Impose BC" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(imposeBCpart);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* "End time-step" */
      peano4::parallel::SpacetreeSet::getInstance().traverse(endTimeStep);
      {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();

      /* Do we need to dump the snapshot? */
      if (i % DeltaSnapshots == 0){{
        logInfo( "main()", "plot timestep " << i );
        peano4::parallel::SpacetreeSet::getInstance().traverse(plotObserver);
        {FULL_NAMESPACE}::vertexdata::ParticleSet::finishedTraversal();
      }}

    }}

    logInfo( "main()", "terminated successfully" )

    /**
     * =======================
     * End of SPH simulation 
     * ======================
     *
     */

  }}
  else {{
    while (peano4::parallel::Node::getInstance().continueToRun()) {{
      logDebug( "runParallel(...)", "trigger a new sweep with step " << peano4::parallel::Node::getInstance().getCurrentProgramStep() );
    }}
  }}
  #if defined(SharedOMP)
  }}
  }}
  #endif

  peano4::shutdownSingletons();
  {FULL_NAMESPACE}::repositories::DataRepository::shutdownDatatypes();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}}
