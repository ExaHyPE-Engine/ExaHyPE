// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{MAIN_NAME}}.h"

#include "{{SUBDIRECTORY}}Constants.h"
#include "{{SUBDIRECTORY}}observers/CreateGridWithRefinement.h"
#include "{{SUBDIRECTORY}}observers/CreateGridWithoutRefinement.h"
#include "{{SUBDIRECTORY}}observers/InitGrid.h"
#include "{{SUBDIRECTORY}}observers/PlotSolution.h"
#include "{{SUBDIRECTORY}}observers/CheckpointSolution.h"
#include "{{SUBDIRECTORY}}observers/TimeStep.h"
{% if SUBDIRECTORIES -%}
{% for subdirectory in SUBDIRECTORIES %}
#include "{{subdirectory}}repositories/DataRepository.h"
#include "{{subdirectory}}repositories/SolverRepository.h"
#include "{{subdirectory}}repositories/StepRepository.h"
{% endfor -%}
{% else %}
#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"
{%- endif %}

#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/MemoryManager.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

#include "peano4/peano4.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include <iomanip>

{% if FENV == True -%}
#if defined(__linux)
#include <cfenv>
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
#endif
{% endif -%}

#ifdef WITH_SCOREP_USER
#include <scorep/SCOREP_User.h>
#else
#define SCOREP_USER_REGION_DEFINE(handle)
#define SCOREP_USER_REGION_BEGIN(handle, name, type)
#define SCOREP_USER_REGION_END(handle)
#endif

using namespace {{NAMESPACE | join("::")}};

namespace {
  tarch::logging::Log _log("");

  tarch::timing::Measurement gridConstructionMeasurement;
  tarch::timing::Measurement timeStepMeasurement;
  tarch::timing::Measurement plotMeasurement;

  static double printNextTimeStep = 0.0;
  static bool haveReceivedNonCriticalAssertion = false;
} // namespace

/**
 * Decide which step to run next
 *
 * ## Control of the parallel grid construction
 *
 *
 * @return continues to run
 */
bool selectNextAlgorithmicStep() {
  static bool                                   gridConstructed                       = false;
  static bool                                   gridInitialised                       = false;
  static double                                 nextMaxPlotTimeStamp                  = {{SUBNAMESPACE}}FirstPlotTimeStamp;
  static double                                 nextMinPlotTimeStamp                  = {{SUBNAMESPACE}}FirstPlotTimeStamp;
  static double                                 nextMaxCheckpointTimeStamp            = {{SUBNAMESPACE}}FirstCheckpointTimeStamp;
  static double                                 nextMinCheckpointTimeStamp            = {{SUBNAMESPACE}}FirstCheckpointTimeStamp;
  static bool                                   haveJustWrittenSnapshot               = false;
  static tarch::la::Vector<DIMENSIONS, double>  minH                                  = tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());
  static int                                    globalNumberOfTrees                   = 0;
  bool                                          continueToSolve                       = true;

  if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::PlotSolution)
    );
    haveReceivedNonCriticalAssertion = true;
    logError("selectNextAlgorithmicStep()", "Noncritical assertion has been triggered in code. Dump final state and terminate.");
  } else if (tarch::hasNonCriticalAssertionBeenViolated()) {
    continueToSolve = false;
  } else if (gridConstructed and not gridInitialised) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::InitGrid)
    );
    gridInitialised = true;
  } else if (not gridConstructed) {
    if (tarch::la::max(peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH()) < tarch::la::max(minH)) {
      minH = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH();
      logDebug("selectNextAlgorithmicStep()", "Mesh has refined, so reset minH to " << minH << " and postpone further refinement.");
      peano4::parallel::Node::getInstance().setNextProgramStep({{SUBNAMESPACE}}repositories::StepRepository::toProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithoutRefinement
      ));
    } else if ({{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfTrees() > globalNumberOfTrees) {
      logDebug("selectNextAlgorithmicStep()", "Mesh has rebalanced recently, so postpone further refinement.");
      peano4::parallel::Node::getInstance().setNextProgramStep({{SUBNAMESPACE}}repositories::StepRepository::toProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithoutRefinement
      ));
      globalNumberOfTrees = {{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfTrees();
    } else if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps() > 5
      and
      // Ensure that a proper creation has been ran before, so the mesh had the opportunity to refine further if it has not done yet.
      {{SUBNAMESPACE}}repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()) == {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithRefinement
    ) {
      logDebug("selectNextAlgorithmicStep()", "Grid has been stationary for quite some time. Terminate grid construction.");
      gridConstructed = true;
    } else {
      logDebug("selectNextAlgorithmicStep()", "Mesh rebalancing seems to be stationary, so study whether to refine mesh further in next sweep.");
      peano4::parallel::Node::getInstance().setNextProgramStep({{SUBNAMESPACE}}repositories::StepRepository::toProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithRefinement)
      );
      globalNumberOfTrees = {{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfTrees();
    }
  } else {
    if (
      {{SUBNAMESPACE}}TimeInBetweenPlots > 0.0
      and
      {{SUBNAMESPACE}}repositories::getMinTimeStamp() < {{SUBNAMESPACE}}MinTerminalTime
      and
      {{SUBNAMESPACE}}repositories::getMaxTimeStamp() < {{SUBNAMESPACE}}MaxTerminalTime
      and
      ({{SUBNAMESPACE}}repositories::getMinTimeStamp() >= nextMinPlotTimeStamp or {{SUBNAMESPACE}}repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
      and
      {{SUBNAMESPACE}}repositories::mayPlot()
    ) {
      if ({{SUBNAMESPACE}}repositories::getMinTimeStamp() >= nextMinPlotTimeStamp) {
        nextMinPlotTimeStamp += {{SUBNAMESPACE}}TimeInBetweenPlots;
      }
      if ({{SUBNAMESPACE}}repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
        nextMaxPlotTimeStamp += {{SUBNAMESPACE}}TimeInBetweenPlots;
      }

      if (nextMinPlotTimeStamp < {{SUBNAMESPACE}}repositories::getMinTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to plot every dt="
            << {{SUBNAMESPACE}}TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << ({{SUBNAMESPACE}}repositories::getMinTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenPlots)
        );
        nextMinPlotTimeStamp = {{SUBNAMESPACE}}repositories::getMinTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenPlots;
      } else if (nextMaxPlotTimeStamp < {{SUBNAMESPACE}}repositories::getMaxTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to plot every dt="
            << {{SUBNAMESPACE}}TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << ({{SUBNAMESPACE}}repositories::getMaxTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenPlots)
        );
        nextMaxPlotTimeStamp = {{SUBNAMESPACE}}repositories::getMaxTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenPlots;
      }

      nextMaxPlotTimeStamp = std::max(nextMaxPlotTimeStamp, nextMinPlotTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::PlotSolution)
      );
      haveJustWrittenSnapshot = true;
    } else if (
      {{SUBNAMESPACE}}TimeInBetweenCheckpoints > 0.0
      and
      {{SUBNAMESPACE}}repositories::getMinTimeStamp() < {{SUBNAMESPACE}}MinTerminalTime
      and
      {{SUBNAMESPACE}}repositories::getMaxTimeStamp() < {{SUBNAMESPACE}}MaxTerminalTime
      and
      ({{SUBNAMESPACE}}repositories::getMinTimeStamp() >= nextMinCheckpointTimeStamp or {{SUBNAMESPACE}}repositories::getMaxTimeStamp() >= nextMaxCheckpointTimeStamp)
      and
      {{SUBNAMESPACE}}repositories::mayPlot()
    ) {
      if ({{SUBNAMESPACE}}repositories::getMinTimeStamp() >= nextMinCheckpointTimeStamp) {
        nextMinCheckpointTimeStamp += {{SUBNAMESPACE}}TimeInBetweenCheckpoints;
      }
      if ({{SUBNAMESPACE}}repositories::getMaxTimeStamp() >= nextMaxCheckpointTimeStamp) {
        nextMaxCheckpointTimeStamp += {{SUBNAMESPACE}}TimeInBetweenCheckpoints;
      }

      if (nextMinCheckpointTimeStamp < {{SUBNAMESPACE}}repositories::getMinTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to checkpoint every dt="
            << {{SUBNAMESPACE}}TimeInBetweenCheckpoints << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next checkpoint to t=" << ({{SUBNAMESPACE}}repositories::getMinTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenCheckpoints)
        );
        nextMinCheckpointTimeStamp = {{SUBNAMESPACE}}repositories::getMinTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenCheckpoints;
      } else if (nextMaxCheckpointTimeStamp < {{SUBNAMESPACE}}repositories::getMaxTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to checkpoint every dt="
            << {{SUBNAMESPACE}}TimeInBetweenCheckpoints << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next checkpoint to t=" << ({{SUBNAMESPACE}}repositories::getMaxTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenCheckpoints)
        );
        nextMaxCheckpointTimeStamp = {{SUBNAMESPACE}}repositories::getMaxTimeStamp() + {{SUBNAMESPACE}}TimeInBetweenCheckpoints;
      }

      nextMaxCheckpointTimeStamp = std::max(nextMaxCheckpointTimeStamp, nextMinCheckpointTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::CheckpointSolution)
      );
      haveJustWrittenSnapshot = true;
    } else if ({{SUBNAMESPACE}}repositories::getMinTimeStamp() < {{SUBNAMESPACE}}MinTerminalTime and {{SUBNAMESPACE}}repositories::getMaxTimeStamp() < {{SUBNAMESPACE}}MaxTerminalTime) {
      peano4::parallel::Node::getInstance().setNextProgramStep(
        {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::TimeStep)
      );
      continueToSolve         = true;
      haveJustWrittenSnapshot = false;
    } else {
      if (not haveJustWrittenSnapshot and {{SUBNAMESPACE}}TimeInBetweenPlots > 0.0 and {{SUBNAMESPACE}}repositories::mayPlot()) {
        peano4::parallel::Node::getInstance().setNextProgramStep(
          {{SUBNAMESPACE}}repositories::StepRepository::toProgramStep({{SUBNAMESPACE}}repositories::StepRepository::Steps::PlotSolution)
        );
        haveJustWrittenSnapshot = true;
        nextMinPlotTimeStamp    = std::numeric_limits<double>::max();
        nextMaxPlotTimeStamp    = std::numeric_limits<double>::max();
      } else if (not haveJustWrittenSnapshot and {{SUBNAMESPACE}}TimeInBetweenPlots > 0.0 and not {{SUBNAMESPACE}}repositories::mayPlot()) {
        continueToSolve = true; // don't want to terminate immediately but to wait for incomplete time steps to complete
      } else {
        continueToSolve = false;
      }
    }
  }

  return continueToSolve;
}


void printNextAlgorithmicStep(const {{SUBNAMESPACE}}repositories::StepRepository::Steps& stepName) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo(
      "printNextAlgorithmicStep()",
      "Starting AlgorithmicStep "
      << "[" << {{SUBNAMESPACE}}repositories::StepRepository::toString(stepName)
      << ", #Trees=" << {{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfTrees()
      << ", #Cells=" << {{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells()
      << "]"
    );
  }
}


void step() {
  static tarch::timing::Watch watch("::", "step()", false);

  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = {{SUBNAMESPACE}}repositories::StepRepository::toStepEnum(stepIdentifier);

  tarch::logging::Statistics::getInstance().log(
    "load-balancing", toolbox::loadbalancing::LoadBalancer::getStatistics()
  );

  tarch::logging::Statistics::getInstance().log(
    "memory", tarch::accelerator::MemoryManager::getStatistics()
  );

  SCOREP_USER_REGION_DEFINE(CreateGrid)
  SCOREP_USER_REGION_DEFINE(CreateGrid_startGridConstructionStep)
  SCOREP_USER_REGION_DEFINE(CreateGrid_traverse)
  SCOREP_USER_REGION_DEFINE(CreateGrid_finishGridConstructionStep)
  SCOREP_USER_REGION_DEFINE(InitGrid)
  SCOREP_USER_REGION_DEFINE(InitGrid_startGridInitialisationStep)
  SCOREP_USER_REGION_DEFINE(InitGrid_prepareTraversal)
  SCOREP_USER_REGION_DEFINE(InitGrid_traverse)
  SCOREP_USER_REGION_DEFINE(InitGrid_unprepareTraversal)
  SCOREP_USER_REGION_DEFINE(InitGrid_finishGridInitialisationStep)
  SCOREP_USER_REGION_DEFINE(PlotSolution)
  SCOREP_USER_REGION_DEFINE(PlotSolution_startPlottingStep)
  SCOREP_USER_REGION_DEFINE(PlotSolution_prepareTraversal)
  SCOREP_USER_REGION_DEFINE(PlotSolution_traverse)
  SCOREP_USER_REGION_DEFINE(PlotSolution_unprepareTraversal)
  SCOREP_USER_REGION_DEFINE(PlotSolution_finishPlottingStep)
  SCOREP_USER_REGION_DEFINE(TimeStep)
  SCOREP_USER_REGION_DEFINE(TimeStep_startTimeStep)
  SCOREP_USER_REGION_DEFINE(TimeStep_prepareTraversal)
  SCOREP_USER_REGION_DEFINE(TimeStep_traverse)
  SCOREP_USER_REGION_DEFINE(TimeStep_unprepareTraversal)
  SCOREP_USER_REGION_DEFINE(TimeStep_finishTimeStep)

  switch (stepName) {
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithoutRefinement: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithoutRefinement");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(CreateGrid, "CreateGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(CreateGrid_startGridConstructionStep, "startGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_startGridConstructionStep)

    SCOREP_USER_REGION_BEGIN(CreateGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::CreateGridWithoutRefinement observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(CreateGrid_traverse)

    SCOREP_USER_REGION_BEGIN(CreateGrid_finishGridConstructionStep, "finishGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_finishGridConstructionStep)

    SCOREP_USER_REGION_END(CreateGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::CreateGridWithRefinement: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithRefinement");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(CreateGrid, "CreateGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(CreateGrid_startGridConstructionStep, "startGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_startGridConstructionStep)

    SCOREP_USER_REGION_BEGIN(CreateGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::CreateGridWithRefinement observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(CreateGrid_traverse)

    SCOREP_USER_REGION_BEGIN(CreateGrid_finishGridConstructionStep, "finishGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_finishGridConstructionStep)

    SCOREP_USER_REGION_END(CreateGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::InitGrid: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("InitGrid");
    printNextAlgorithmicStep(stepName);
    {{SUBNAMESPACE}}repositories::loadBalancer.enable(false);

    watch.start();
    SCOREP_USER_REGION_BEGIN(InitGrid, "InitGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(InitGrid_startGridInitialisationStep, "startGridInitialisationStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startGridInitialisationStep();
    SCOREP_USER_REGION_END(InitGrid_startGridInitialisationStep)

    SCOREP_USER_REGION_BEGIN(InitGrid_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::InitGrid::prepareTraversal();
    SCOREP_USER_REGION_END(InitGrid_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(InitGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::InitGrid observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(InitGrid_traverse)

    SCOREP_USER_REGION_BEGIN(InitGrid_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::InitGrid::unprepareTraversal();
    SCOREP_USER_REGION_END(InitGrid_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(InitGrid_finishGridInitialisationStep, "finishGridInitialisationStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishGridInitialisationStep();
    SCOREP_USER_REGION_END(InitGrid_finishGridInitialisationStep)

    SCOREP_USER_REGION_END(InitGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::PlotSolution: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("PlotSolution");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(PlotSolution, "PlotSolution", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(PlotSolution_startPlottingStep, "startPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_startPlottingStep)

    SCOREP_USER_REGION_BEGIN(PlotSolution_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::PlotSolution::prepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_prepareTraversal)

     SCOREP_USER_REGION_BEGIN(PlotSolution_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::PlotSolution observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(PlotSolution_traverse)

    SCOREP_USER_REGION_BEGIN(PlotSolution_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::PlotSolution::unprepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_finishPlottingStep, "finishPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_finishPlottingStep)

    SCOREP_USER_REGION_END(PlotSolution)
    watch.stop();
    plotMeasurement.setValue(watch.getCalendarTime());
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::CheckpointSolution: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CheckpointSolution");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(PlotSolution, "PlotSolution", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(PlotSolution_startPlottingStep, "startPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_startPlottingStep)

    SCOREP_USER_REGION_BEGIN(PlotSolution_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::CheckpointSolution::prepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::CheckpointSolution observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(PlotSolution_traverse)

    SCOREP_USER_REGION_BEGIN(PlotSolution_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::CheckpointSolution::unprepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_finishPlottingStep, "finishPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_finishPlottingStep)

    SCOREP_USER_REGION_END(PlotSolution)
    watch.stop();
    plotMeasurement.setValue(watch.getCalendarTime());
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::TimeStep: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("TimeStep");

    watch.start();
    SCOREP_USER_REGION_BEGIN(TimeStep, "TimeStep", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(TimeStep_startTimeStep, "startTimeStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::startTimeStep();
    SCOREP_USER_REGION_END(TimeStep_startTimeStep)

    SCOREP_USER_REGION_BEGIN(TimeStep_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::TimeStep::prepareTraversal();
    SCOREP_USER_REGION_END(TimeStep_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(TimeStep_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::TimeStep observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(TimeStep_traverse)

    SCOREP_USER_REGION_BEGIN(TimeStep_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}observers::TimeStep::unprepareTraversal();
    SCOREP_USER_REGION_END(TimeStep_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(TimeStep_finishTimeStep, "finishTimeStep", SCOREP_USER_REGION_TYPE_COMMON)
    {{SUBNAMESPACE}}repositories::finishTimeStep();
    SCOREP_USER_REGION_END(TimeStep_finishTimeStep)

    SCOREP_USER_REGION_END(TimeStep)
    watch.stop();
    timeStepMeasurement.setValue(watch.getCalendarTime());

    if (
      tarch::mpi::Rank::getInstance().isGlobalMaster()
      and
      timeStepMeasurement.getAccumulatedValue() >= printNextTimeStep
      and
      {{SUBNAMESPACE}}repositories::mayPlot()
    ) {
      printNextAlgorithmicStep(stepName);
      {{SUBNAMESPACE}}repositories::printTimeStep(false);
      if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
        logInfo(
          "step()",
          "Average cell updates per second: "
          << {{SUBNAMESPACE}}repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells() / timeStepMeasurement.getValue()
        );
      }
      printNextTimeStep += {{SUBNAMESPACE}}repositories::PrintTimeStepIntervalInSeconds;
    }
  } break;
  case {{SUBNAMESPACE}}repositories::StepRepository::Steps::Undef:
    assertion(false);
    break;
  }
}

int main(int argc, char** argv) {
  int returnCode = EXIT_SUCCESS;
  peano4::init(&argc, &argv, {{SUBNAMESPACE}}DomainOffset, {{SUBNAMESPACE}}DomainSize, {{SUBNAMESPACE}}PeriodicBC);

#ifdef WITH_TARGETDART
  td_init((void *) &main);
#endif

  {% if FENV == True -%}
#if defined(__linux)
  //feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif
  {% endif -%}

  {{SUBNAMESPACE}}repositories::initLogFilters();
  {{SUBNAMESPACE}}repositories::DataRepository::initDatatypes();
  {{SUBNAMESPACE}}repositories::startSimulation();

  tarch::logging::Statistics::getInstance().clear();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    while (selectNextAlgorithmicStep()) {
      step();
    }

    if (haveReceivedNonCriticalAssertion) {
      logWarning("main()", "Terminated gracefully after noncritical assertion");
      returnCode = EXIT_FAILURE;
    } else {
      // Print final timestep information
      {{SUBNAMESPACE}}repositories::printTimeStep(false);
      logInfo("main()", "Terminated successfully");
      returnCode = EXIT_SUCCESS;
    }

    if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Average cell updates per second: " << repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells() / timeStepMeasurement.getValue());
      logInfo("main()", "Average time per mesh sweep:     " << timeStepMeasurement.getValue() << "s\t");
      logInfo("main()", "Time stepping:                   " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString());
    }
    if (gridConstructionMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Initial grid construction:       " << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString());
    }
    if (plotMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Plotting:                        " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString());
    }
  } else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      step();
    }
  }

  tarch::logging::Statistics::getInstance().writeToCSV();

  {{SUBNAMESPACE}}repositories::finishSimulation();
  {{SUBNAMESPACE}}repositories::DataRepository::shutdownDatatypes();

  peano4::shutdown();

  return returnCode;
}
