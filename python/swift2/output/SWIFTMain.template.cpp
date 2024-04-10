#include "{{MAIN_NAME}}.h"
#include "Constants.h"

#include "tarch/NonCriticalAssertions.h"
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
#include "toolbox/particles/UnitTests.h"

#include "swift2/UserInterface.h"

#include "Constants.h"

#include "tarch/logging/LogFilterFileReader.h"

{% if FENV_ARGS is defined and FENV_ARGS != "" -%}
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
{% endif -%}

#include "observers/CreateGrid.h"
#include "observers/InitialConditions.h"
#include "observers/Plot.h"

{% for STEP in SOLVERSTEP_NAMES %}
#include "observers/{{STEP}}.h"
{% endfor %}

{% for STEP in INITIALISATIONSTEP_NAMES %}
#include "observers/{{STEP}}.h"
{% endfor %}


#include "tarch/timing/Measurement.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log _log("::");


tarch::timing::Measurement   createGridMeasurement;
tarch::timing::Measurement   initialConditionMeasurement;
tarch::timing::Measurement   initialisationMeasurement;
tarch::timing::Measurement   plotMeasurement;
tarch::timing::Measurement   timeStepMeasurement;


bool {{NAMESPACE | join("::")}}::selectNextAlgorithmicStep() {
  static bool   gridConstructed                             = false;
  static double nextMaxPlotTimeStamp                        = {{NAMESPACE | join("::")}}::FirstPlotTimeStamp;
  static double nextMinPlotTimeStamp                        = {{NAMESPACE | join("::")}}::FirstPlotTimeStamp;
  static tarch::la::Vector<Dimensions,double> minH          = tarch::la::Vector<Dimensions,double>( std::numeric_limits<double>::max() );
  bool          continueToSolve                             = true;
  static bool   haveReceivedNoncriticialAssertion           = false;

  // ==============================================================
  // Phase x: Generic types of steps for all phases:
  //          - Rerun previous sweep again;
  //          - Dump result due to non-critical assertion.
  // ==============================================================
  if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNoncriticialAssertion) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
    );
    haveReceivedNoncriticialAssertion = true;
    logError(
      "selectNextAlgorithmicStep()", "non-critical assertion has been triggered in code. Dump final state and terminate"
    );
  }
  else if (tarch::hasNonCriticalAssertionBeenViolated()) {
    continueToSolve = false;
  }
  else if ( repositories::rerunPreviousGridSweep() ) {
    logInfo(
      "selectNextAlgorithmicStep()",
      "rerun previous step " << repositories::StepRepository::toString(
        repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())
      )
    );
  }
  // ==============================
  // Phase 1: Set up (initial) grid
  // ==============================
  else if (not gridConstructed) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGrid )
    );

    if (
      tarch::la::max( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH() ) < GlobalMaxH
    ) {
      logInfo(
        "selectNextAlgorithmicStep()",
        "h_min=" << tarch::la::max( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH() ) <<
        ", which his smaller than global h_max=" << GlobalMaxH <<
        ". Continue with load balancing "
      );
    }
    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()>5
    ) {
      logInfo(
        "selectNextAlgorithmicStep()",
        "mesh has been stationary for more than 5 grid sweeps. Stop grid construction"
      );
      gridConstructed = true;
    }
  }
  // ==============================
  // Phase 2: Insert the particles
  // ==============================
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::CreateGrid
    and
    gridConstructed
    and
    not repositories::loadBalancer.hasSplitRecently()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::InitialConditions )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::CreateGrid
    and
    gridConstructed
    and
    repositories::loadBalancer.hasSplitRecently()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::CreateGrid )
    );
  }
  // ======================================================
  // Phase 3: Run through the particle initialisation steps
  // ======================================================
  //
  // Phase 3.a: Switch from initialisation (particle insertion) to first initialisation step
  {% if INITIALISATIONSTEP_NAMES|length>0 %}
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::InitialConditions
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[0]}} )
    );
  }
  {% endif %}
  // Phase 3.b: Run through individual initialisation steps
  {% for STEP_NUMBER in range(0,INITIALISATIONSTEP_NAMES|length-1) %}
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER+1]}} )
    );
  }
  {% endfor %}
  // Phase 3.c: If last step has finished and is not to be repeated, switch to
  // time step or plot
  else if (
    {% if INITIALISATIONSTEP_NAMES|length>0 %}
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[-1]}}
    {% else %}
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::InitialConditions
    {% endif %}
    and
    TimeInBetweenPlots>0.0
  ) {
    logInfo(
      "selectNextAlgorithmicStep()",
      "plot initial condition"
    );
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
    );
  }
  else if (
    {% if INITIALISATIONSTEP_NAMES|length>0 %}
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[-1]}}
    {% else %}
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::InitialConditions
    {% endif %}
  ) {
    logInfo(
      "selectNextAlgorithmicStep()",
      "no plotting active, switch to first time step immediately"
    );
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[0]}} )
    );
  }
  // ============================================
  // Phase 4: Run through the particle time steps
  // ============================================
  //
  // Phase 4.a: Initial substeps
  {% for STEP_NUMBER in range(0,SOLVERSTEP_NAMES|length-1) %}
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[STEP_NUMBER]}}
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[STEP_NUMBER+1]}} )
    );
  }
  {% endfor %}
  // Phase 4.b: Final step might switch back to 0 or plot. It might also terminate if we don't plot
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[-1]}}
    and
    ( repositories::getMinTimeStamp()>=MinTerminalTime or repositories::getMaxTimeStamp()>=MaxTerminalTime )
    and
    ( TimeInBetweenPlots>0.0 )
  ) {
    logInfo(
      "selectNextAlgorithmicStep()",
      "plot final solution"
    );
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[-1]}}
    and
    ( repositories::getMinTimeStamp()>=MinTerminalTime or repositories::getMaxTimeStamp()>=MaxTerminalTime )
  ) {
    continueToSolve         = false;
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[-1]}}
    and
    TimeInBetweenPlots>0.0
    and
    (repositories::getMinTimeStamp()>=nextMinPlotTimeStamp or repositories::getMaxTimeStamp()>=nextMaxPlotTimeStamp)
  ) {
    if (repositories::getMinTimeStamp()>=nextMinPlotTimeStamp) {
      nextMinPlotTimeStamp += TimeInBetweenPlots;
    }
    if (repositories::getMaxTimeStamp()>=nextMaxPlotTimeStamp) {
      nextMaxPlotTimeStamp += TimeInBetweenPlots;
    }

    if ( nextMinPlotTimeStamp < repositories::getMinTimeStamp() ) {
      logWarning(
        "selectNextAlgorithmicStep()",
        "code is asked to plot every dt=" << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. " <<
        "So postpone next plot to t=" << (repositories::getMinTimeStamp() + TimeInBetweenPlots)
      );
      nextMinPlotTimeStamp = repositories::getMinTimeStamp() + TimeInBetweenPlots;
    }
    else if ( nextMaxPlotTimeStamp < repositories::getMaxTimeStamp() ) {
      logWarning(
        "selectNextAlgorithmicStep()",
        "code is asked to plot every dt=" << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. " <<
        "So postpone next plot to t=" << (repositories::getMaxTimeStamp() + TimeInBetweenPlots)
      );
      nextMaxPlotTimeStamp = repositories::getMaxTimeStamp() + TimeInBetweenPlots;
    }

    nextMaxPlotTimeStamp = std::max(nextMaxPlotTimeStamp,nextMinPlotTimeStamp);

    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[-1]}}
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[0]}} )
    );
  }
  // ======================================================
  // Phase 5: Switch back from plotting to time stepping
  // ======================================================
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::Plot
    and
    ( repositories::getMinTimeStamp()>=MinTerminalTime or repositories::getMaxTimeStamp()>=MaxTerminalTime )
  ) {
    continueToSolve         = false;
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::Plot
   ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[0]}} )
    );
  }
  else {
    logError( "selectNextAlgorithmicStep", "no switch variant found" );
    assertion(false);
  }

  return continueToSolve;
}


tarch::timing::Watch  watch( "::", "step()", false );


void {{NAMESPACE | join("::")}}::step() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  static tarch::logging::Log _log("");
  #if PeanoDebug>0
  #else
  if (tarch::mpi::Rank::getInstance().isGlobalMaster())
  #endif
  logInfo( "step()", "run " << repositories::StepRepository::toString(stepName) );

  switch ( stepName ) {
    case repositories::StepRepository::Steps::CreateGrid:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid" );
        watch.start();
        observers::CreateGrid  observer;
        observers::CreateGrid::prepareTraversal();
        repositories::startGridConstructionStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishGridConstructionStep();
        observers::CreateGrid::unprepareTraversal();
        watch.stop();
        createGridMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::InitialConditions:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "initial-conditions" );
        watch.start();
        OTTER_PHASE_SWITCH("initial-conditions");
        observers::InitialConditions  observer;
        observers::InitialConditions::prepareTraversal();
        repositories::startInitialisationStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishInitialisationStep();
        observers::InitialConditions::unprepareTraversal();
        watch.stop();
        initialConditionMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Plot:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot" );
        watch.start();
        OTTER_PHASE_SWITCH("plot");
        observers::Plot  observer;
        observers::Plot::prepareTraversal();
        repositories::startPlotStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishPlotStep();
        observers::Plot::unprepareTraversal();
        watch.stop();
        plotMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[0]}}:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "time-step" );
        watch.start();
        OTTER_PHASE_SWITCH("{{SOLVERSTEP_NAMES[0]}}");
        observers::{{SOLVERSTEP_NAMES[0]}}  observer;
        observers::{{SOLVERSTEP_NAMES[0]}}::prepareTraversal();
        repositories::startTimeStep();
        repositories::startIntermediateStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::{{SOLVERSTEP_NAMES[0]}}::unprepareTraversal();
      }
      break;
    {% for STEP_NUMBER in range(1,SOLVERSTEP_NAMES|length-1) %}
    case repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[STEP_NUMBER]}}:
      {
        OTTER_PHASE_SWITCH("{{SOLVERSTEP_NAMES[STEP_NUMBER]}}");
        repositories::startIntermediateStep();
        observers::{{SOLVERSTEP_NAMES[STEP_NUMBER]}}  observer;
        observers::{{SOLVERSTEP_NAMES[STEP_NUMBER]}}::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::{{SOLVERSTEP_NAMES[STEP_NUMBER]}}::unprepareTraversal();
      }
      break;
    {% endfor %}
    case repositories::StepRepository::Steps::{{SOLVERSTEP_NAMES[-1]}}:
      {
        OTTER_PHASE_SWITCH("{{SOLVERSTEP_NAMES[-1]}}");
        observers::{{SOLVERSTEP_NAMES[-1]}}  observer;
        observers::{{SOLVERSTEP_NAMES[-1]}}::prepareTraversal();
        repositories::startIntermediateStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        repositories::finishTimeStep();
        observers::{{SOLVERSTEP_NAMES[-1]}}::unprepareTraversal();
        watch.stop();
        timeStepMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    {% for STEP_NUMBER in range(0,INITIALISATIONSTEP_NAMES|length) %}
    case repositories::StepRepository::Steps::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}:
      {
        OTTER_PHASE_SWITCH("{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}");
        repositories::startInitialisationStep();
        observers::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}  observer;
        observers::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishInitialisationStep();
        observers::{{INITIALISATIONSTEP_NAMES[STEP_NUMBER]}}::unprepareTraversal();
        watch.stop();
        initialisationMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    {% endfor %}
    case repositories::StepRepository::Steps::Undef:
      assertion(false);
      break;
  }
}



int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeUnitTestsFailed  = 1;
  const int ExitCodeInvalidArguments = 2;

  {% if FENV_ARGS is defined and FENV_ARGS != "" -%}
  feenableexcept({{FENV_ARGS}} );
  {% endif -%}

  peano4::initParallelEnvironment(&argc,&argv);
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();
  tarch::initNonCriticalAssertionEnvironment();

  peano4::initSingletons(
    {{NAMESPACE | join("::")}}::DomainOffset,
    {{NAMESPACE | join("::")}}::DomainSize,
    {{NAMESPACE | join("::")}}::PeriodicBC
  );

  {{NAMESPACE | join("::")}}::repositories::DataRepository::initDatatypes();

  if (not swift2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  #if PeanoDebug>=2
  tarch::tests::TestCase* peanoCoreTests = peano4::getUnitTests();
  peanoCoreTests->run();
  if (peanoCoreTests->getNumberOfErrors() != 0) {
    logError("main()", "Peano4 core unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete peanoCoreTests;

  tarch::tests::TestCase* tarchTests = tarch::getUnitTests();
  tarchTests->run();
  if (tarchTests->getNumberOfErrors() != 0) {
    logError("main()", "technical architecture (tarch) unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete tarchTests;

  tarch::tests::TestCase* swiftTests = swift2::getUnitTests();
  swiftTests->run();
  if (swiftTests->getNumberOfErrors() != 0) {
    logError("main()", "Swift unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete swiftTests;

  tarch::tests::TestCase* particlesTests = toolbox::particles::getUnitTests();
  particlesTests->run();
  if (particlesTests->getNumberOfErrors() != 0) {
    logError("main()", "toolbox particles unit tests failed. Quit.");
    exit(ExitCodeUnitTestsFailed);
  }
  delete particlesTests;
  #endif

  tarch::logging::Statistics::getInstance().clear();

  OTTER_INITIALISE();


  #if defined(SharedOMP)
  #pragma omp parallel
  {
  #pragma omp master
  {
  #endif
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    while ( {{NAMESPACE | join("::")}}::selectNextAlgorithmicStep() ) {
      {{NAMESPACE | join("::")}}::step();
    }

    logInfo("main()", "terminated successfully");
    logInfo("main()", "grid construction:  " << createGridMeasurement.getAccumulatedValue() << "s\t" << createGridMeasurement.toString() );
    logInfo("main()", "initial conditions: " << initialConditionMeasurement.getAccumulatedValue() << "s\t" << initialConditionMeasurement.toString() );
    logInfo("main()", "initialisation:     " << initialisationMeasurement.getAccumulatedValue() << "s\t" << initialisationMeasurement.toString() );
    logInfo("main()", "plotting:           " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString() );
    logInfo("main()", "time stepping:      " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString() );
  }
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      {{NAMESPACE | join("::")}}::step();
    }
  }
  #if defined(SharedOMP)
  }
  }
  #endif

  OTTER_FINALISE();

  peano4::shutdownSingletons();
  {{NAMESPACE | join("::")}}::repositories::DataRepository::shutdownDatatypes();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}

