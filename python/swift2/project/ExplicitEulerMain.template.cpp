#include "{MAIN_NAME}.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/multicore.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/GlobalState.h"

#include "peano4/UnitTests.h"
#include "tarch/UnitTests.h"
#include "swift2/UnitTests.h"

#include "Constants.h"

#include "toolbox/loadbalancing/RecursiveSubdivision.h"
#include "tarch/logging/LogFilterFileReader.h"

#ifdef UseFEExcept
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
#endif


#include "observers/CreateGrid.h"
#include "observers/Init.h"
#include "observers/Plot.h"
#include "observers/TimeStep.h"
#include "observers/CalculateForce.h"


#include "tarch/timing/Measurement.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log _log("::");


tarch::timing::Measurement   createGridMeasurement;
tarch::timing::Measurement   initMeasurement;
tarch::timing::Measurement   plotMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   calculateForceMeasurement;


using namespace {FULL_NAMESPACE};


bool selectNextAlgorithmicStep() {{
  static bool   gridConstructed                             = false;
  static bool   gridInitialised                             = false;
  static double nextMaxPlotTimeStamp                        = FirstPlotTimeStamp;
  static double nextMinPlotTimeStamp                        = FirstPlotTimeStamp;
  static bool   haveJustWrittenSnapshot                     = false;
  static tarch::la::Vector<Dimensions,double> minH          = tarch::la::Vector<Dimensions,double>( std::numeric_limits<double>::max() );
  static int    globalNumberOfTrees                         = 0;
  bool          continueToSolve                             = true;

  if ( repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::CalculateForce ) {{
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::TimeStep )
    );
    continueToSolve = true;
  }}
  else if (not gridConstructed) {{
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGrid )
    );

    if (
      tarch::la::max( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH() ) < {FULL_NAMESPACE}::MaxH
      or
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()>5
    ) {{
      gridConstructed = true;
    }}
  }}
  else if (not gridInitialised) {{
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Init )
    );

    gridInitialised = true;
  }}
  else {{
    if (
      TimeInBetweenPlots>0.0 and repositories::getMinTimeStamp()<MinTerminalTime and repositories::getMaxTimeStamp()<MaxTerminalTime
      and
      repositories::getMinTimeStamp()>=nextMinPlotTimeStamp or repositories::getMaxTimeStamp()>=nextMaxPlotTimeStamp
    ) {{
      if (repositories::getMinTimeStamp()>=nextMinPlotTimeStamp) {{
        nextMinPlotTimeStamp += TimeInBetweenPlots;
      }}
      if (repositories::getMaxTimeStamp()>=nextMaxPlotTimeStamp) {{
        nextMaxPlotTimeStamp += TimeInBetweenPlots;
      }}

      if ( nextMinPlotTimeStamp < repositories::getMinTimeStamp() ) {{
        logWarning(
          "selectNextAlgorithmicStep()",
          "code is asked to plot every dt=" << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. " <<
          "So postpone next plot to t=" << (repositories::getMinTimeStamp() + TimeInBetweenPlots)
        );
        nextMinPlotTimeStamp = repositories::getMinTimeStamp() + TimeInBetweenPlots;
      }}
      else if ( nextMaxPlotTimeStamp < repositories::getMaxTimeStamp() ) {{
        logWarning(
          "selectNextAlgorithmicStep()",
          "code is asked to plot every dt=" << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. " <<
          "So postpone next plot to t=" << (repositories::getMaxTimeStamp() + TimeInBetweenPlots)
        );
        nextMaxPlotTimeStamp = repositories::getMaxTimeStamp() + TimeInBetweenPlots;
      }}

      nextMaxPlotTimeStamp = std::max(nextMaxPlotTimeStamp,nextMinPlotTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
      );
      haveJustWrittenSnapshot = true;
      continueToSolve         = true;
    }}
    else if ( repositories::getMinTimeStamp()<MinTerminalTime and repositories::getMaxTimeStamp()<MaxTerminalTime ) {{
      peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CalculateForce )
      );
      continueToSolve         = true;
      haveJustWrittenSnapshot = false;
    }}
    else {{
      if (not haveJustWrittenSnapshot and TimeInBetweenPlots>0.0 ) {{
        peano4::parallel::Node::getInstance().setNextProgramStep(
          repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
        );
        continueToSolve         = true; // don't want to terminate immediately
        haveJustWrittenSnapshot = true;
        nextMinPlotTimeStamp    = std::numeric_limits<double>::max();
        nextMaxPlotTimeStamp    = std::numeric_limits<double>::max();
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

  switch ( stepName ) {{
    case repositories::StepRepository::Steps::CreateGrid:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid" );
        observers::CreateGrid  observer;
        repositories::startGridConstructionStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishGridConstructionStep();
        watch.stop();
        createGridMeasurement.setValue( watch.getCalendarTime() );
      }}
      break;
    case repositories::StepRepository::Steps::Init:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "init" );
        otterPhaseBegin( "init" );
        observers::Init  observer;
        repositories::startInitialisationStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishInitialisationStep();
        watch.stop();
        initMeasurement.setValue( watch.getCalendarTime() );
      }}
      break;
    case repositories::StepRepository::Steps::Plot:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot" );
        otterPhaseSwitch( "plot" );
        observers::Plot  observer;
        repositories::startPlotStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishPlotStep();
        watch.stop();
        plotMeasurement.setValue( watch.getCalendarTime() );
      }}
      break;
    case repositories::StepRepository::Steps::TimeStep:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "time-step" );
        otterPhaseSwitch( "time-step" );
        observers::TimeStep  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishTimeStep();
        watch.stop();
        timeStepMeasurement.setValue( watch.getCalendarTime() );
      }}
      break;
    case repositories::StepRepository::Steps::CalculateForce:
      {{
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "calculate-force" );
        otterPhaseSwitch( "calculate-force" );
        observers::CalculateForce  observer;
        repositories::startTimeStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        calculateForceMeasurement.setValue( watch.getCalendarTime() );
      }}
      break;
    case repositories::StepRepository::Steps::Undef:
      assertion(false);
      break;
  }}
}}



int main(int argc, char** argv) {{
  const int ExitCodeSuccess         = 0;
  const int ExitCodeUnitTestsFailed = 1;

#ifdef UseFEExcept
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  peano4::initParallelEnvironment(&argc,&argv);
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    PeriodicBC
  );

/*
  if (not exahype2::parseCommandLineArguments(argc,argv) ) {{
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }}
*/

  {FULL_NAMESPACE}::repositories::DataRepository::initDatatypes();

  std::string logFilterFile = "swift2.log-filter";
  if ( not tarch::logging::LogFilterFileReader::parsePlainTextFile( logFilterFile ) ) {{
    logWarning( "main()", "no swift2.log-filter file found or file has been corrupted. Use default logging configuration" );
  }}


  #if PeanoDebug>=2
  tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {{
    logError("main()", "Peano4 core unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }}
  delete peanoCoreTests;

  tarch::tests::TestCase* tarchTests = tarch::getUnitTests();
  tarchTests->run();
  if (tarchTests->getNumberOfErrors() != 0) {{
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }}
  delete tarchTests;

  tarch::tests::TestCase* swiftTests = swift2::getUnitTests();
  swiftTests->run();
  if (swiftTests->getNumberOfErrors() != 0) {{
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }}
  delete swiftTests;
  #endif

  tarch::logging::Statistics::getInstance().clear();

  otterTraceInitialise(OTTER_SRC_ARGS());


  #if defined(SharedOMP)
  #pragma omp parallel
  {{
  #pragma omp master 
  {{
  #endif
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {{
    while ( selectNextAlgorithmicStep() ) {{
      step();
    }}

    logInfo("main()", "terminated successfully");
    logInfo("main()", "grid construction:  " << createGridMeasurement.getAccumulatedValue() << "s\t" << createGridMeasurement.toString() );
    logInfo("main()", "init simulation:    " << initMeasurement.getAccumulatedValue() << "s\t" << initMeasurement.toString() );
    logInfo("main()", "plotting:           " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString() );
    logInfo("main()", "time stepping:      " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString() );
    logInfo("main()", "force calculation:  " << calculateForceMeasurement.getAccumulatedValue() << "s\t" << calculateForceMeasurement.toString() );
  }}
  else {{
    while (peano4::parallel::Node::getInstance().continueToRun()) {{
      step();
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
