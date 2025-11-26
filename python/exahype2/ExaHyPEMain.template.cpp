// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{MAIN_NAME}}.h"

#include "Constants.h"
#include "observers/CreateGridWithRefinement.h"
#include "observers/CreateGridWithoutRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/CheckpointSolution.h"
#include "observers/TimeStep.h"
#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

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

#include "exahype2/exahype2.h"

#include <iomanip>
#include <sstream>

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
  static double                                 nextMaxPlotTimeStamp                  = FirstPlotTimeStamp;
  static double                                 nextMinPlotTimeStamp                  = FirstPlotTimeStamp;
  static double                                 nextMaxCheckpointTimeStamp            = FirstCheckpointTimeStamp;
  static double                                 nextMinCheckpointTimeStamp            = FirstCheckpointTimeStamp;
  static bool                                   haveJustWrittenSnapshot               = false;
  static tarch::la::Vector<DIMENSIONS, double>  minH                                  = tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());
  static int                                    globalNumberOfTrees                   = 0;
  bool                                          continueToSolve                       = true;
  const auto                                    statistics                            = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();

  if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
    haveReceivedNonCriticalAssertion = true;
    logError("selectNextAlgorithmicStep()", "Noncritical assertion has been triggered in code. Dump final state and terminate.");
  } else if (tarch::hasNonCriticalAssertionBeenViolated()) {
    continueToSolve = false;
  } else if (gridConstructed and not gridInitialised) {
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::InitGrid));
    gridInitialised = true;
  } else if (not gridConstructed) {
    if (tarch::la::max(statistics.minH) < tarch::la::max(minH)) {
      minH = statistics.minH;
      logDebug("selectNextAlgorithmicStep()", "Mesh has refined, so reset minH to " << minH << " and postpone further refinement.");
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    } else if (repositories::loadBalancer.getGlobalNumberOfTrees() > globalNumberOfTrees) {
      logDebug("selectNextAlgorithmicStep()", "Mesh has rebalanced recently, so postpone further refinement.");
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
      globalNumberOfTrees = repositories::loadBalancer.getGlobalNumberOfTrees();
    } else if (
      statistics.numberOfStationarySweeps > 5
      and
      // Ensure that a proper creation has been ran before, so the mesh had the opportunity to refine further if it has not done yet.
      repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()) == repositories::StepRepository::Steps::CreateGridWithRefinement
    ) {
      logDebug("selectNextAlgorithmicStep()", "Grid has been stationary for quite some time. Terminate grid construction.");
      gridConstructed = true;
    } else {
      logDebug("selectNextAlgorithmicStep()", "Mesh rebalancing seems to be stationary, so study whether to refine mesh further in next sweep.");
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithRefinement));
      globalNumberOfTrees = repositories::loadBalancer.getGlobalNumberOfTrees();
    }
  } else {
    if (
      TimeInBetweenPlots > 0.0
      and
      repositories::getMinTimeStamp() < MinTerminalTime
      and
      repositories::getMaxTimeStamp() < MaxTerminalTime
      and
      (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
      and
      repositories::mayPlot()
    ) {
      nextMinPlotTimeStamp += TimeInBetweenPlots;
      nextMaxPlotTimeStamp += TimeInBetweenPlots;

      if (nextMinPlotTimeStamp < repositories::getMinTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to plot every dt="
            << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << (repositories::getMinTimeStamp() + TimeInBetweenPlots)
        );
        nextMinPlotTimeStamp = repositories::getMinTimeStamp() + TimeInBetweenPlots;
      } else if (nextMaxPlotTimeStamp < repositories::getMaxTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to plot every dt="
            << TimeInBetweenPlots << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << (repositories::getMaxTimeStamp() + TimeInBetweenPlots)
        );
        nextMaxPlotTimeStamp = repositories::getMaxTimeStamp() + TimeInBetweenPlots;
      }

      nextMaxPlotTimeStamp = std::max(nextMaxPlotTimeStamp, nextMinPlotTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
      haveJustWrittenSnapshot = true;
    } else if (
      TimeInBetweenCheckpoints > 0.0
      and
      repositories::getMinTimeStamp() < MinTerminalTime
      and
      repositories::getMaxTimeStamp() < MaxTerminalTime
      and
      (repositories::getMinTimeStamp() >= nextMinCheckpointTimeStamp or repositories::getMaxTimeStamp() >= nextMaxCheckpointTimeStamp)
      and
      repositories::mayPlot()
    ) {
      nextMinCheckpointTimeStamp += TimeInBetweenCheckpoints;
      nextMaxCheckpointTimeStamp += TimeInBetweenCheckpoints;

      if (nextMinCheckpointTimeStamp < repositories::getMinTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to checkpoint every dt="
            << TimeInBetweenCheckpoints << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next checkpoint to t=" << (repositories::getMinTimeStamp() + TimeInBetweenCheckpoints)
        );
        nextMinCheckpointTimeStamp = repositories::getMinTimeStamp() + TimeInBetweenCheckpoints;
      } else if (nextMaxCheckpointTimeStamp < repositories::getMaxTimeStamp()) {
        logWarning(
          "selectNextAlgorithmicStep()",
          "Code is asked to checkpoint every dt="
            << TimeInBetweenCheckpoints << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next checkpoint to t=" << (repositories::getMaxTimeStamp() + TimeInBetweenCheckpoints)
        );
        nextMaxCheckpointTimeStamp = repositories::getMaxTimeStamp() + TimeInBetweenCheckpoints;
      }

      nextMaxCheckpointTimeStamp = std::max(nextMaxCheckpointTimeStamp, nextMinCheckpointTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CheckpointSolution));
      haveJustWrittenSnapshot = true;
    } else if (repositories::getMinTimeStamp() < MinTerminalTime and repositories::getMaxTimeStamp() < MaxTerminalTime) {
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::TimeStep));
      continueToSolve         = true;
      haveJustWrittenSnapshot = false;
    } else {
      if (not haveJustWrittenSnapshot and TimeInBetweenPlots > 0.0 and repositories::mayPlot()) {
        peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
        haveJustWrittenSnapshot = true;
        nextMinPlotTimeStamp    = std::numeric_limits<double>::max();
        nextMaxPlotTimeStamp    = std::numeric_limits<double>::max();
      } else if (not haveJustWrittenSnapshot and TimeInBetweenPlots > 0.0 and not repositories::mayPlot()) {
        continueToSolve = true; // don't want to terminate immediately but to wait for incomplete time steps to complete
      } else {
        continueToSolve = false;
      }
    }
  }

  return continueToSolve;
}


void printNextAlgorithmicStep(const repositories::StepRepository::Steps& stepName, bool printStatistics = false) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("printNextAlgorithmicStep()", "Starting AlgorithmicStep " << "[" << repositories::StepRepository::toString(stepName) << "]");

    if (printStatistics) {
      const auto minLoad = repositories::loadBalancer.getGlobalMinTreeLoad();
      const auto maxLoad = repositories::loadBalancer.getGlobalMaxTreeLoad();
      const auto loadPct = repositories::loadBalancer.getGlobalTreeLoadRatioInPercentage();

      const auto memoryUsedByGrid       = repositories::loadBalancer.getGlobalMemoryUsageInBytes();
      const auto memoryUsedByHeap       = repositories::DataRepository::getGlobalMemoryUsageInBytes();
      const auto totalPersistentMemory  = memoryUsedByGrid + memoryUsedByHeap;

      std::ostringstream msg;
      msg << "#Patches="  << repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells()
          << ", #DoFs="   << repositories::getNumberOfDoFs()
          << ", #Trees="  << repositories::loadBalancer.getGlobalNumberOfTrees()
          << ", #Load="   << std::fixed << std::setprecision(1) << loadPct << "% (min=" << minLoad << ", max=" << maxLoad << ")"
          << ", #Memory=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(totalPersistentMemory, tarch::chooseMemoryUsageFormat(totalPersistentMemory)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(totalPersistentMemory))
          << " (Grid="    << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByGrid, tarch::chooseMemoryUsageFormat(memoryUsedByGrid)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByGrid))
          << ", Heap="    << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByHeap, tarch::chooseMemoryUsageFormat(memoryUsedByHeap)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByHeap))
          << ")";
      logInfo("printNextAlgorithmicStep()", msg.str());

      if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
        const auto dofs = repositories::getNumberOfDoFs();
        const auto globalNumberOfPatches = repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells();
        const auto totalNumberOfDoFs = dofs * globalNumberOfPatches;
        logInfo("printNextAlgorithmicStep()", "Patch updates per second: " << globalNumberOfPatches / timeStepMeasurement.getMostRecentValue() << ", DoF updates per second: " << totalNumberOfDoFs / timeStepMeasurement.getMostRecentValue());
      }
    }
  }
}


void step() {
  static tarch::timing::Watch watch("::", "step()", false);

  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

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
  case repositories::StepRepository::Steps::CreateGridWithoutRefinement: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithoutRefinement");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(CreateGrid, "CreateGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(CreateGrid_startGridConstructionStep, "startGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_startGridConstructionStep)

    SCOREP_USER_REGION_BEGIN(CreateGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::CreateGridWithoutRefinement observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(CreateGrid_traverse)

    SCOREP_USER_REGION_BEGIN(CreateGrid_finishGridConstructionStep, "finishGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_finishGridConstructionStep)

    SCOREP_USER_REGION_END(CreateGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();
  } break;
  case repositories::StepRepository::Steps::CreateGridWithRefinement: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CreateGridWithRefinement");
    printNextAlgorithmicStep(stepName, true);

    watch.start();
    SCOREP_USER_REGION_BEGIN(CreateGrid, "CreateGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(CreateGrid_startGridConstructionStep, "startGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_startGridConstructionStep)

    SCOREP_USER_REGION_BEGIN(CreateGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::CreateGridWithRefinement observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(CreateGrid_traverse)

    SCOREP_USER_REGION_BEGIN(CreateGrid_finishGridConstructionStep, "finishGridConstructionStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishGridConstructionStep();
    SCOREP_USER_REGION_END(CreateGrid_finishGridConstructionStep)

    SCOREP_USER_REGION_END(CreateGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();
  } break;
  case repositories::StepRepository::Steps::InitGrid: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("InitGrid");
    printNextAlgorithmicStep(stepName, true);
    repositories::loadBalancer.enable(false);

    watch.start();
    SCOREP_USER_REGION_BEGIN(InitGrid, "InitGrid", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(InitGrid_startGridInitialisationStep, "startGridInitialisationStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startGridInitialisationStep();
    SCOREP_USER_REGION_END(InitGrid_startGridInitialisationStep)

    SCOREP_USER_REGION_BEGIN(InitGrid_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::InitGrid::prepareTraversal();
    SCOREP_USER_REGION_END(InitGrid_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(InitGrid_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::InitGrid observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(InitGrid_traverse)

    SCOREP_USER_REGION_BEGIN(InitGrid_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::InitGrid::unprepareTraversal();
    SCOREP_USER_REGION_END(InitGrid_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(InitGrid_finishGridInitialisationStep, "finishGridInitialisationStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishGridInitialisationStep();
    SCOREP_USER_REGION_END(InitGrid_finishGridInitialisationStep)

    SCOREP_USER_REGION_END(InitGrid)
    watch.stop();
    gridConstructionMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();
  } break;
  case repositories::StepRepository::Steps::PlotSolution: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("PlotSolution");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(PlotSolution, "PlotSolution", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(PlotSolution_startPlottingStep, "startPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_startPlottingStep)

    SCOREP_USER_REGION_BEGIN(PlotSolution_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::PlotSolution::prepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_prepareTraversal)

     SCOREP_USER_REGION_BEGIN(PlotSolution_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::PlotSolution observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(PlotSolution_traverse)

    SCOREP_USER_REGION_BEGIN(PlotSolution_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::PlotSolution::unprepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_finishPlottingStep, "finishPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_finishPlottingStep)

    SCOREP_USER_REGION_END(PlotSolution)
    watch.stop();
    plotMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();
  } break;
  case repositories::StepRepository::Steps::CheckpointSolution: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("CheckpointSolution");
    printNextAlgorithmicStep(stepName);

    watch.start();
    SCOREP_USER_REGION_BEGIN(PlotSolution, "PlotSolution", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(PlotSolution_startPlottingStep, "startPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_startPlottingStep)

    SCOREP_USER_REGION_BEGIN(PlotSolution_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::CheckpointSolution::prepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::CheckpointSolution observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(PlotSolution_traverse)

    SCOREP_USER_REGION_BEGIN(PlotSolution_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::CheckpointSolution::unprepareTraversal();
    SCOREP_USER_REGION_END(PlotSolution_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(PlotSolution_finishPlottingStep, "finishPlottingStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishPlottingStep();
    SCOREP_USER_REGION_END(PlotSolution_finishPlottingStep)

    SCOREP_USER_REGION_END(PlotSolution)
    watch.stop();
    plotMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();
  } break;
  case repositories::StepRepository::Steps::TimeStep: {
    tarch::logging::LogFilter::getInstance().switchProgramPhase("TimeStep");

    watch.start();
    SCOREP_USER_REGION_BEGIN(TimeStep, "TimeStep", SCOREP_USER_REGION_TYPE_COMMON)

    SCOREP_USER_REGION_BEGIN(TimeStep_startTimeStep, "startTimeStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::startTimeStep();
    SCOREP_USER_REGION_END(TimeStep_startTimeStep)

    SCOREP_USER_REGION_BEGIN(TimeStep_prepareTraversal, "prepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::TimeStep::prepareTraversal();
    SCOREP_USER_REGION_END(TimeStep_prepareTraversal)

    SCOREP_USER_REGION_BEGIN(TimeStep_traverse, "traverse", SCOREP_USER_REGION_TYPE_COMMON)
    observers::TimeStep observer;
    peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
    SCOREP_USER_REGION_END(TimeStep_traverse)

    SCOREP_USER_REGION_BEGIN(TimeStep_unprepareTraversal, "unprepareTraversal", SCOREP_USER_REGION_TYPE_COMMON)
    observers::TimeStep::unprepareTraversal();
    SCOREP_USER_REGION_END(TimeStep_unprepareTraversal)

    SCOREP_USER_REGION_BEGIN(TimeStep_finishTimeStep, "finishTimeStep", SCOREP_USER_REGION_TYPE_COMMON)
    repositories::finishTimeStep();
    SCOREP_USER_REGION_END(TimeStep_finishTimeStep)

    SCOREP_USER_REGION_END(TimeStep)
    watch.stop();
    // TODO: We need to be careful here. If we couple solvers the TimeStep observer does not reflect one complete time step.
    // Therefore, this measurement does not give us the information we want!
    // The same applies for any solver that needs multiple sweeps.
    timeStepMeasurement.setValue(watch.getCalendarTime());

    repositories::DataRepository::updateMemoryUsage();

    if (
      tarch::mpi::Rank::getInstance().isGlobalMaster()
      and
      timeStepMeasurement.getAccumulatedValue() >= printNextTimeStep
      and
      repositories::mayPlot()
    ) {
      printNextAlgorithmicStep(stepName, true);
      repositories::printTimeStep(false);
      printNextTimeStep += repositories::PrintTimeStepIntervalInSeconds;
    }
  } break;
  case repositories::StepRepository::Steps::Undef:
    assertion(false);
    break;
  }
}

int main(int argc, char** argv) {
  int returnCode = EXIT_SUCCESS;

  peano4::init(&argc, &argv, DomainOffset, DomainSize, PeriodicBC);

  peano4::writeCopyrightMessage();
  ::exahype2::writeCopyrightMessage();

{% if FENV == True -%}
#if defined(__linux)
  //feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif
{% endif -%}

  repositories::initLogFilters();
  repositories::DataRepository::init();
  repositories::startSimulation();

  tarch::logging::Statistics::getInstance().clear();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    while (selectNextAlgorithmicStep()) {
      peano4::parallel::Node::getInstance().continueToRun();
      step();
    }

    peano4::parallel::Node::getInstance().setNextProgramStep(peano4::parallel::Node::Terminate);
    peano4::parallel::Node::getInstance().continueToRun(); // Send terminate instruction to other ranks

    if (haveReceivedNonCriticalAssertion) {
      logWarning("main()", "Terminated gracefully after noncritical assertion");
      returnCode = EXIT_FAILURE;
    } else {
      // Print final timestep information
      repositories::printTimeStep(false);
      logInfo("main()", "Terminated successfully");
      returnCode = EXIT_SUCCESS;
    }

    if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Average patch updates per second:  " << repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells() / timeStepMeasurement.getValue());
      logInfo("main()", "Average DoF updates per second:    " << (repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells() *  repositories::getNumberOfDoFs()) / timeStepMeasurement.getValue());
      logInfo("main()", "Time stepping:                     " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString());
    }
    if (gridConstructionMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Initial grid construction:         " << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString());
    }
    if (plotMeasurement.getNumberOfMeasurements() > 0) {
      logInfo("main()", "Plotting:                          " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString());
    }
  } else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      step();
    }
  }

  tarch::logging::Statistics::getInstance().writeToCSV();

  repositories::finishSimulation();
  repositories::DataRepository::shutdown();

  peano4::shutdown();

  return returnCode;
}
