#include "{MAIN_NAME}.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/UnitTests.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/UnitTests.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/SolverRepository.h"

#include "observers/CreateGrid.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "toolbox/loadbalancing/loadbalancing.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"


#if defined(UseSmartMPI)
#endif


using namespace {FULL_NAMESPACE};



tarch::logging::Log _log("::");



tarch::timing::Measurement   gridConstructionMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   plotMeasurement;


/**
 *
 * <h2> Control of the parallel grid construction </h2>
 *
 * We realise a very simplistic parallel grid construction strategy: When I run
 * the grid creation, I allow refinement if no rebalancing has happened, and I 
 * stop the grid construction as soon as I've met the minimal mesh width. 
 * Otherwise, I use refinement. There's exceptions to this:
 *
 * - Every @K = f$ 3^d @f$ iterations, I run a refinement even if I rebalance. It 
 *   might happen that rebalancing is extremely slow or not successful on one 
 *   rank, making another drop in, and so forth. So I use K, as 
 *   there are at most that many splits possible due to a grid refinement.
 *   K might be chosen slightly smaller (see code).
 * - In each step, I identify whether I've changed the minimal mesh. If not, 
 *   I increment a counter. If this counter is $3K$, I know that the mesh 
 *   refinement would have had the opportunity to refine the grid three times.
 *   It hasn't done so. So I can assume that the minimal mesh size not yet 
 *   yet reached, but the mesh refinement criteria don't need that minimal mesh
 *   (yet). Such effects arise for smooth boundary conditions, e.g., that 
 *   create shocks later on, i.e. they arise for non-linear PDEs.
 * 
 * @return continues to run
 */
bool selectNextAlgorithmicStep() {{
  static bool   gridConstructed                             = false;
  static bool   gridInitialised                             = false;
  static bool   gridBalanced                                = false;
  static double nextPlotTimeStamp                           = FirstPlotTimeStamp;
  static bool   haveJustWrittenSnapshot                     = false;
  static bool   haveReceivedNoncriticialAssertion           = false;
  static bool   addGridSweepWithoutGridRefinementNext       = false;
  static tarch::la::Vector<Dimensions,double> minH          = tarch::la::Vector<Dimensions,double>( std::numeric_limits<double>::max() );
  static int    globalNumberOfTrees                         = 0;
  static int    stationaryGridCreationSteps                 = 0;
  bool          continueToSolve                             = true;
  
  if (exahype2::hasNonCriticalAssertionBeenViolated() and not haveReceivedNoncriticialAssertion) {{
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::PlotSolution )
    );
    haveReceivedNoncriticialAssertion = true;
    logError( "selectNextAlgorithmicStep()", "non-critical assertion has been triggered in code. Dump final state and terminate" );
  }}
  else if (exahype2::hasNonCriticalAssertionBeenViolated()) {{
    continueToSolve = false;
  }}
  else if (gridConstructed and not gridBalanced) {{
    if (
      not repositories::loadBalancer.isEnabled(true)
      and
      not repositories::loadBalancer.hasSplitRecently()
    ) {{
      logInfo( "selectNextAlgorithmicStep()", "all ranks have switched off their load balancing" );
      gridBalanced = true;
    }}

    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGridAndConvergeLoadBalancing )
    );

    assertionNumericalEquals( repositories::getMinTimeStamp(), 0.0 );
  }}
  else if (gridBalanced and not gridInitialised) {{
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::InitGrid )
    );

    gridInitialised = true;
 
    assertionNumericalEquals( repositories::getMinTimeStamp(), 0.0 );
  }}
  else if (not gridConstructed) {{
    // Grid construction termination criterion
    if (
      tarch::la::max( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH() ) <= repositories::getMinMeshSize()
      and
      repositories::loadBalancer.getGlobalNumberOfTrees()<=globalNumberOfTrees
    ) {{
      logInfo( "selectNextAlgorithmicStep()", "finest mesh resolution of " << repositories::getMinMeshSize() << " reached." );
      gridConstructed = true;
      addGridSweepWithoutGridRefinementNext = false;
      globalNumberOfTrees = repositories::loadBalancer.getGlobalNumberOfTrees();
    }}
    else if ( tarch::la::max( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH() ) < tarch::la::max( minH ) ) {{
      minH = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH();
      logDebug( "selectNextAlgorithmicStep()", "mesh has refined, so reset minH=" << minH << " and postpone further refinement" );
      addGridSweepWithoutGridRefinementNext = true;
    }}
    else if (repositories::loadBalancer.getGlobalNumberOfTrees()>globalNumberOfTrees) {{
      logInfo( 
        "selectNextAlgorithmicStep()", 
        "mesh has rebalanced recently, so postpone further refinement)" 
      );
      addGridSweepWithoutGridRefinementNext = true;
      globalNumberOfTrees = repositories::loadBalancer.getGlobalNumberOfTrees();
    }} 
    else if (stationaryGridCreationSteps>5) {{
      logInfo( "selectNextAlgorithmicStep()", "grid has been stationary for quite some time. Terminate grid construction" );
      addGridSweepWithoutGridRefinementNext = false;
      gridConstructed = true;
    }}
    else {{
      logInfo( "selectNextAlgorithmicStep()", "mesh rebalancing seems to be kind of stationary, so study whether to refine mesh further in next sweep" );
      addGridSweepWithoutGridRefinementNext = false;

      if ( repositories::loadBalancer.getGlobalNumberOfTrees()<=globalNumberOfTrees ) {{
        stationaryGridCreationSteps++;
      }}
      else {{
        stationaryGridCreationSteps=0;
      }}

      globalNumberOfTrees = repositories::loadBalancer.getGlobalNumberOfTrees();
    }}

    // Actual grid traversal choice
    if (addGridSweepWithoutGridRefinementNext) {{
      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGridButPostponeRefinement )
      );
    }}
    else {{
      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGrid )
      );
    }}

    continueToSolve = true;
  }}
  else {{
    if ( repositories::getMinTimeStamp()>=nextPlotTimeStamp  and TimeInBetweenPlots>0.0 and repositories::getMinTimeStamp()<TerminalTime ) {{
      nextPlotTimeStamp += TimeInBetweenPlots;
      if ( nextPlotTimeStamp < repositories::getMinTimeStamp() ) {{
        logWarning( "selectNextAlgorithmicStep()", "code is asked to plot every dt=" << TimeInBetweenPlots << ", but this seems to be less than the minimal time step size of the solvers. Next plot will be due at t=" << nextPlotTimeStamp );
        nextPlotTimeStamp = repositories::getMinTimeStamp() + TimeInBetweenPlots;
      }}
      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::PlotSolution )
      );
      haveJustWrittenSnapshot = true;
      continueToSolve         = true;
    }}
    else if ( repositories::getMinTimeStamp()<TerminalTime ) {{
      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::TimeStep )
      );
      continueToSolve         = true;
      haveJustWrittenSnapshot = false;
    }}
    else {{
      if (not haveJustWrittenSnapshot and TimeInBetweenPlots>0.0) {{
        peano4::parallel::Node::getInstance().setNextProgramStep(
          repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::PlotSolution )
        );
        continueToSolve         = true; // don't want to terminate immediately
        haveJustWrittenSnapshot = true;
        nextPlotTimeStamp       = std::numeric_limits<double>::max();
      }}
      else {{
        continueToSolve = false;
      }}
    }}
  }}

  return continueToSolve;
}}


void step() {{
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  static tarch::logging::Log _log("");
  #if PeanoDebug>0
  #else
  if (tarch::mpi::Rank::getInstance().isGlobalMaster())
  #endif
  logInfo( "step()", "run " << repositories::StepRepository::toString(stepName) );

  tarch::timing::Watch  watch( "::", "step()", false );

  static int creepingNumberOfLocalCells = 0;

  switch ( stepName ) {{
    case repositories::StepRepository::Steps::CreateGridButPostponeRefinement:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid-but-postpone-refinement" );
        repositories::startGridConstructionStep();

        observers::CreateGridButPostponeRefinement  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        gridConstructionMeasurement.setValue( watch.getCalendarTime() );

        repositories::finishGridConstructionStep();
      }}
      break;
    case repositories::StepRepository::Steps::CreateGrid:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid" );
        repositories::startGridConstructionStep();
        
        observers::CreateGrid  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        gridConstructionMeasurement.setValue( watch.getCalendarTime() );

        repositories::finishGridConstructionStep();

        // We always overestimate so give the convergence the opportunity to catch up. The constant 
        // here is a magic one. 
        creepingNumberOfLocalCells = ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree()+tarch::multicore::Core::getInstance().getNumberOfThreads()*3;
      }}
      break;
    case repositories::StepRepository::Steps::CreateGridAndConvergeLoadBalancing:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid-and-converge-load-balancing" );
        // The smaller here corresponds to the -1 below
        if (
          ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree()<0
          and
          repositories::loadBalancer.isEnabled(false)
        ) {{
          logInfo( "step()", "rank is degenerated so disable load balancing temporarily" );
          repositories::loadBalancer.enable(false);
        }}
        if (
          ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree() >= creepingNumberOfLocalCells
          and
          repositories::loadBalancer.isEnabled(false)
        ) {{
          logInfo( "step()", "grid initialisation on this rank seems to be stable, disable load balancing temporarily" );
          repositories::loadBalancer.enable(false);
        }}

        repositories::startGridConstructionStep();

        observers::CreateGridButPostponeRefinement  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        gridConstructionMeasurement.setValue( watch.getCalendarTime() );

        repositories::finishGridConstructionStep();

        if (
          ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree() <= creepingNumberOfLocalCells
	  and
	  not repositories::loadBalancer.hasSplitRecently()
          and
          repositories::loadBalancer.isEnabled(false)
        ) {{
          logInfo(
            "step()",
            "have to decrement local cell counter " << creepingNumberOfLocalCells <<
            " as maximum weight is " << ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree()
          );
          creepingNumberOfLocalCells = (creepingNumberOfLocalCells + ::toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree())/2;
        }}
      }}
      break;
    case repositories::StepRepository::Steps::InitGrid:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "init-grid" );
        repositories::loadBalancer.enable(false);

        repositories::startGridInitialisationStep();
        
        observers::InitGrid  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        gridConstructionMeasurement.setValue( watch.getCalendarTime() );

        repositories::finishGridInitialisationStep();
      }}
      break;
    case repositories::StepRepository::Steps::PlotSolution:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot-solution" );
        const double minTimeStamp    = repositories::getMinTimeStamp();
        const double maxTimeStamp    = repositories::getMaxTimeStamp();
        const double minTimeStepSize = repositories::getMinTimeStepSize();
        const double maxTimeStepSize = repositories::getMaxTimeStepSize();

        repositories::startPlottingStep( minTimeStamp, maxTimeStamp, minTimeStepSize, maxTimeStepSize );

        observers::PlotSolution  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        plotMeasurement.setValue( watch.getCalendarTime() );
        
        repositories::finishPlottingStep();
      }}
      break;
    case repositories::StepRepository::Steps::TimeStep:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "time-step" );
        if (repositories::loadBalancer.isEnabled(false)) {{
          logInfo( "step()", "disable load balancing throughout initialisation (to be removed in later releases)" );
          repositories::loadBalancer.enable(false);
        }}

        const double minTimeStamp    = repositories::getMinTimeStamp();
        const double maxTimeStamp    = repositories::getMaxTimeStamp();
        const double minTimeStepSize = repositories::getMinTimeStepSize();
        const double maxTimeStepSize = repositories::getMaxTimeStepSize();

        if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {{
          logInfo( "step()", "t_{{min}}  = " << minTimeStamp );
          logInfo( "step()", "t_{{max}}  = " << maxTimeStamp );
          logInfo( "step()", "dt_{{min}} = " << minTimeStepSize );
          logInfo( "step()", "dt_{{max}} = " << maxTimeStepSize );
        }}
        repositories::startTimeStep( minTimeStamp, maxTimeStamp, minTimeStepSize, maxTimeStepSize );
        
        observers::TimeStep  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        timeStepMeasurement.setValue( watch.getCalendarTime() );
        
        repositories::finishTimeStep();
      }}
      break;
    case repositories::StepRepository::Steps::Undef:
      assertion(false);
      break;
  }}
}}
  
  
int main(int argc, char** argv) {{
  const int ExitCodeSuccess          = 0;
  const int ExitCodeUnitTestsFailed  = 1;
  const int ExitCodeInvalidArguments = 2;
  const int ExitCodeInvalidBuild     = 3;
  
  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();
 
  
  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    {PERIODIC_BC}
  );
  
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {{
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }}

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {{
    return ExitCodeInvalidArguments;
  }}
  
  {FULL_NAMESPACE}::repositories::DataRepository::initDatatypes();
  
  #if PeanoDebug>=2
  tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {{
    logError("main()", "Peano4 core unit tests failed. Quit.");
    tarch::mpi::Rank::abort( ExitCodeUnitTestsFailed );
  }}
  delete peanoCoreTests;

  tarch::tests::TestCase* peanoTarchTests = tarch::getUnitTests();
  peanoTarchTests->run();
  if (peanoTarchTests->getNumberOfErrors() != 0) {{
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    tarch::mpi::Rank::abort( ExitCodeUnitTestsFailed );
  }}
  delete peanoTarchTests;
  #endif

  repositories::startSimulation();

  tarch::logging::Statistics::getInstance().clear();

  #if defined(SharedOMP)
  #pragma omp parallel
  {{
  #pragma omp master 
  {{
  #endif
	  
	  
  #if defined(UseSmartMPI)
  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster() and smartmpi::isComputeRank();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster() and smartmpi::isComputeRank();
  #else
  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();
  #endif
	  
	  
  if ( isGlobalMaster ) {{
    while ( selectNextAlgorithmicStep() ) {{
      step();
    }}

    logInfo("main()", "terminated successfully");
    logInfo("main()", "initial grid construction:  " << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString() );
    logInfo("main()", "plotting:                   " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString() );
    logInfo("main()", "time stepping:              " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString() );
  }}
  else if (isPeanoComputeNode) {{
    while (peano4::parallel::Node::getInstance().continueToRun()) {{
      step();
    }}
  }}
  #if defined(UseSmartMPI)
  else {{
    while ( smartmpi::continueToRun() ) {{
      smartmpi::tick();
    }}
  }}
  #endif
  
  #if defined(SharedOMP)
  }}
  }}
  #endif

  tarch::logging::Statistics::getInstance().writeToCSV();

  repositories::finishSimulation();

  peano4::shutdownSingletons();
  {FULL_NAMESPACE}::repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}}
