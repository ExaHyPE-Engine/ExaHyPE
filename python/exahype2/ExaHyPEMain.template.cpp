// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{MAIN_NAME}}.h"

#include "Constants.h"
#include "observers/CreateGridWithRefinement.h"
#include "observers/CreateGridWithoutRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
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

using namespace {{NAMESPACE | join("::")}};

namespace {
  tarch::logging::Log _log("");

  tarch::timing::Measurement gridConstructionMeasurement;
  tarch::timing::Measurement gridInitialisationMeasurement;
  tarch::timing::Measurement timeStepMeasurement;
  tarch::timing::Measurement plotMeasurement;

  static double logNextTimeStep = 0.0;
  static bool haveReceivedNonCriticalAssertion = false;
} // namespace

/**
 * Evaluates the state of the mesh and decides if construction/refinement loops should continue.
 *
 * @return true if construction should continue (sets next step implicitly), false if construction is done.
 */
bool shouldContinueGridConstruction(double& currentMinH, int& currentGlobalNumberOfTrees) {
  assertion(repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees() > 0);
  static bool finishConditionSeenLastSweep = false;

  // If the mesh size has decreased, we pause refinement to let the grid settle.
  if (tarch::la::smaller(repositories::loadBalancer.getStatistics().getGlobalMinH(), currentMinH)) {
    [[maybe_unused]] const double oldH  = currentMinH; // Only for logging
    currentMinH                         = repositories::loadBalancer.getStatistics().getGlobalMinH();
    finishConditionSeenLastSweep        = false;
    const auto globalNumberOfLeaves     = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto maxDepth                 = repositories::loadBalancer.getStatistics().getGlobalMaxDepth();
    logInfo("shouldContinueGridConstruction()", "Mesh refinement detected (h_{min} decreased: " << oldH << " -> " << currentMinH << "), Leaves=" << globalNumberOfLeaves << ", Levels=" << maxDepth);
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  // If new trees (subdomains) have appeared, we pause refinement to let the load balancer settle.
  if (repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees() > currentGlobalNumberOfTrees) {
    const auto newTreeCount = repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees();
    logInfo("shouldContinueGridConstruction()", "Rebalancing detected (Tree count increased: " << currentGlobalNumberOfTrees << " -> " << newTreeCount << ")");
    currentGlobalNumberOfTrees = newTreeCount;
    finishConditionSeenLastSweep = false;
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  const bool hasReachedGridConstructionConvergence =
    tarch::la::smallerEquals(currentMinH, repositories::getMinAdmissibleMeshSize())
    and
    repositories::loadBalancer.isStagnating()
    and
    // Ensure that a proper creation has been ran before, so the mesh had the opportunity to refine further if it has not done yet.
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()) == repositories::StepRepository::Steps::CreateGridWithRefinement;

  // If the grid is stable (small enough and converged), terminate grid construction only after one confirmed globally stationary refinement sweep.
  if (hasReachedGridConstructionConvergence) {
    // TODO: This is needed to ensure we have a converged grid creation (especially for AMR). However, with periodic BCs this currently leads to a deadlock.
    // Therefore, we don't use this at the moment (set to true). This should be used as soon as periodic BCs work across subdomains.
    const bool hasConfirmedStationarySweep = /*repositories::loadBalancer.getStatistics().getGlobalNumberOfStationarySweeps() > 0;*/ true;
    if (finishConditionSeenLastSweep and hasConfirmedStationarySweep) {
      logInfo("shouldContinueGridConstruction()", "Grid construction complete: Admissible mesh size reached (h_{min}=" << currentMinH << ", admissible h_{min}=" << repositories::getMinAdmissibleMeshSize() << ") and load balancing has converged");
      return false;
    }

    finishConditionSeenLastSweep = true;
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithRefinement));
    return true;
  }

  finishConditionSeenLastSweep = false;

  // If we are here, the mesh is stationary but not yet fine enough.
  peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithRefinement));
  return true;
}

bool selectNextAlgorithmicStep() {
  static bool    gridConstructed         = false;
  static bool    gridInitialised         = false;
  static double  nextMaxPlotTimeStamp    = FirstPlotTimeStamp;
  static double  nextMinPlotTimeStamp    = FirstPlotTimeStamp;
  static bool    haveJustWrittenSnapshot = false;
  static double  minH                    = std::numeric_limits<double>::max();
  static int     globalNumberOfTrees     = 0;
  bool           continueToSolve         = true;

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
    if (not shouldContinueGridConstruction(minH, globalNumberOfTrees)) {
      gridConstructed = true;
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

void logGlobalStatistics() {
  const auto globalNumberOfLeaves = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells());

  {
    const auto globalNumberOfVertices         = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfVertices());
    const auto globalNumberOfHangingVertices  = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfHangingVertices());
    const auto totalNumberOfVertices          = globalNumberOfVertices + globalNumberOfHangingVertices;
    const auto minDepth                       = repositories::loadBalancer.getStatistics().getGlobalMinDepth();
    const auto maxDepth                       = repositories::loadBalancer.getStatistics().getGlobalMaxDepth();
    std::ostringstream msg;
    msg << "Leaves=" << globalNumberOfLeaves
        << ", Vertices=" << totalNumberOfVertices << " (Local=" << globalNumberOfVertices << ", Hanging=" << globalNumberOfHangingVertices << ")"
        << ", Levels=" << maxDepth << " (min=" << minDepth << ", max=" << maxDepth << ")";
    logInfo("logTopology()", msg.str());
  }

  {
    const auto minLoad                    = repositories::loadBalancer.getStatistics().getGlobalMinTreeLoad();
    const auto maxLoad                    = repositories::loadBalancer.getStatistics().getGlobalMaxTreeLoad();
    const auto efficiency                 = (maxLoad > 0) ? (static_cast<double>(minLoad) / maxLoad) * 100.0 : 100.0;
    const auto globalNumberOfOuterLeaves  = static_cast<double>(repositories::loadBalancer.getStatistics().getGlobalNumberOfOuterUnrefinedCells());
    const auto overhead                   = (globalNumberOfLeaves > 0) ? (globalNumberOfOuterLeaves / globalNumberOfLeaves) * 100.0 : 0.0;
    const auto globalNumberOfTrees        = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees());
    std::ostringstream msg;
    msg << "Trees=" << globalNumberOfTrees
        << ", Efficiency=" << std::fixed << std::setprecision(2) << efficiency << "% (min=" << minLoad << ", max=" << maxLoad << ")"
        << ", Overhead=" << std::fixed << std::setprecision(2) << overhead << "%";
    logInfo("logLoadBalancing()", msg.str());
  }

  {
    const auto memoryUsedByGrid       = repositories::loadBalancer.getStatistics().getGlobalMemoryUsageInBytes();
    const auto memoryUsedByCellData   = repositories::DataRepository::getGlobalCellDataMemoryUsageInBytes();
    const auto memoryUsedByFaceData   = repositories::DataRepository::getGlobalFaceDataMemoryUsageInBytes();
    const auto memoryUsedByVertexData = repositories::DataRepository::getGlobalVertexDataMemoryUsageInBytes();
    const auto totalPersistentMemory  = memoryUsedByGrid + memoryUsedByCellData+ memoryUsedByFaceData + memoryUsedByVertexData;
    std::ostringstream msg;
    msg << std::fixed << std::setprecision(2) << tarch::convertMemorySize(totalPersistentMemory, tarch::chooseMemoryUsageFormat(totalPersistentMemory)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(totalPersistentMemory))
        << " (Grid=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByGrid, tarch::chooseMemoryUsageFormat(memoryUsedByGrid)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByGrid))
        << ", CellData=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByCellData, tarch::chooseMemoryUsageFormat(memoryUsedByCellData)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByCellData))
        << ", FaceData=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByFaceData, tarch::chooseMemoryUsageFormat(memoryUsedByFaceData)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByFaceData))
        << ", VertexData=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByVertexData, tarch::chooseMemoryUsageFormat(memoryUsedByVertexData)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByVertexData))
        << ")";
    logInfo("logMemoryUsage()", msg.str());
  }

  tarch::logging::Statistics::getInstance().log("MemoryUsage", tarch::accelerator::MemoryManager::getStatistics());
  tarch::logging::Statistics::getInstance().log("LoadBalancing", toolbox::loadbalancing::LoadBalancer::getLocalLoadBalancing());

  if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
    const auto dofs               = static_cast<unsigned long>(repositories::getNumberOfDoFs());
    const auto totalNumberOfDoFs  = dofs * globalNumberOfLeaves;
    const auto dt                 = timeStepMeasurement.getMostRecentValue();
    const auto patchesPerSecond   = globalNumberOfLeaves / dt;
    const auto dofsPerSecond      = totalNumberOfDoFs    / dt;
    logInfo("logThroughput()",
      "Patch updates per second: " << patchesPerSecond
      << ", DoF updates per second: " << dofsPerSecond
    );
  }
}

template <class Observer>
void performAlgorithmicStep(
  repositories::StepRepository::Steps stepName,
  std::function<void()>               startStepHook,
  std::function<void()>               finishStepHook,
  tarch::timing::Measurement&         measurement
) {
  tarch::logging::LogFilter::getInstance().switchProgramPhase(repositories::StepRepository::toString(stepName));

  static tarch::timing::Watch watch("::", "performAlgorithmicStep()", false);
  watch.start();

  startStepHook();
  Observer::prepareTraversal();

  Observer observer;
  peano4::parallel::SpacetreeSet::getInstance().traverse(observer);

  Observer::unprepareTraversal();
  finishStepHook();

  watch.stop();
  measurement.setValue(watch.getCalendarTime());
}

void runNextAlgorithmicStep() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  switch (stepName) {
  case repositories::StepRepository::Steps::CreateGridWithoutRefinement:
    performAlgorithmicStep<observers::CreateGridWithoutRefinement>(
      stepName,
      [](){ repositories::startGridConstructionStep(); },
      [](){ repositories::finishGridConstructionStep(); },
      gridConstructionMeasurement
    );
    break;
  case repositories::StepRepository::Steps::CreateGridWithRefinement:
    performAlgorithmicStep<observers::CreateGridWithRefinement>(
      stepName,
      [](){ repositories::startGridConstructionStep(); },
      [](){ repositories::finishGridConstructionStep(); },
      gridConstructionMeasurement
    );
    break;
  case repositories::StepRepository::Steps::InitGrid:
    repositories::loadBalancer.enable(false); // TODO: Remove (dynamic AMR)
    performAlgorithmicStep<observers::InitGrid>(
      stepName,
      [](){ repositories::startGridInitialisationStep(); },
      [](){ repositories::finishGridInitialisationStep(); },
      gridInitialisationMeasurement
    );
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      logInfo("performAlgorithmicStep()", "Grid initialisation complete")
    }
    break;
  case repositories::StepRepository::Steps::PlotSolution:
    performAlgorithmicStep<observers::PlotSolution>(
      stepName,
      [](){ repositories::startPlottingStep(); },
      [](){ repositories::finishPlottingStep(); },
      plotMeasurement
    );
    break;
  case repositories::StepRepository::Steps::TimeStep:
    performAlgorithmicStep<observers::TimeStep>(
      stepName,
      [](){ repositories::startTimeStep(); },
      [](){ repositories::finishTimeStep(); },
      timeStepMeasurement
    );
    if (timeStepMeasurement.getAccumulatedValue() >= logNextTimeStep and repositories::mayPlot()) {
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        repositories::logTimeStep();
        logGlobalStatistics();
      }
      logNextTimeStep += repositories::LogTimeStepIntervalInSeconds;
    }
    break;
  case repositories::StepRepository::Steps::Undef:
    assertion(false);
    break;
  }
}

void setupEnvironment(int argc, char** argv) {
  peano4::init(&argc, &argv, DomainOffset, DomainSize, PeriodicBC);

  peano4::writeCopyrightMessage();
  ::exahype2::writeCopyrightMessage();

  repositories::initLogFilters();
  const int numberOfMaxGlobalSpacetrees = repositories::loadBalancer.getMaxPermittedTreesPerRank() * tarch::mpi::Rank::getInstance().getNumberOfRanks();
  repositories::DataRepository::setNumberOfMaxGlobalSpacetrees(numberOfMaxGlobalSpacetrees);
  repositories::DataRepository::init();
  repositories::startSimulation();

  tarch::logging::Statistics::getInstance().clear();
}

void shutdownEnvironment() {
  tarch::logging::Statistics::getInstance().writeToCSV();

  repositories::finishSimulation();
  repositories::DataRepository::shutdown();

  peano4::shutdown();
}

void logFinalStatistics() {
  if (gridConstructionMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("logFinalStatistics()", "Grid construction:                 " << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString());
  }
  if (gridInitialisationMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("logFinalStatistics()", "Grid initialisation:               " << gridInitialisationMeasurement.getAccumulatedValue() << "s\t" << gridInitialisationMeasurement.toString());
  }
  if (plotMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("logFinalStatistics()", "Plotting:                          " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString());
  }
  if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
    const auto dofs                   = static_cast<double>(repositories::getNumberOfDoFs());
    const auto globalNumberOfLeaves   = static_cast<double>(repositories::loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto totalNumberOfDoFs      = dofs * globalNumberOfLeaves;
    const auto dt                     = timeStepMeasurement.getValue();
    const auto patchesPerSecond       = globalNumberOfLeaves / dt;
    const auto dofsPerSecond          = totalNumberOfDoFs    / dt;
    logInfo("logFinalStatistics()", "Time stepping:                     " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString());
    logInfo("logFinalStatistics()", "Average patch updates per second:  " << patchesPerSecond);
    logInfo("logFinalStatistics()", "Average DoF updates per second:    " << dofsPerSecond);
  }
}

int runGlobalMaster() {
  while (selectNextAlgorithmicStep()) {
    peano4::parallel::Node::getInstance().continueToRun();
    runNextAlgorithmicStep();
  }

  peano4::parallel::Node::getInstance().setNextProgramStep(peano4::parallel::Node::Terminate);
  peano4::parallel::Node::getInstance().continueToRun(); // Send terminate instruction to other ranks

  if (haveReceivedNonCriticalAssertion) {
    logWarning("runGlobalMaster()", "Terminated gracefully after noncritical assertion");
    return EXIT_FAILURE;
  } else {
    repositories::logTimeStep();
    logInfo("runGlobalMaster()", "Terminated successfully");
    logFinalStatistics();
    return EXIT_SUCCESS;
  }
}

void runWorker() {
  while (peano4::parallel::Node::getInstance().continueToRun()) {
    runNextAlgorithmicStep();
  }
}

int main(int argc, char** argv) {
  setupEnvironment(argc, argv);

  int returnCode = EXIT_SUCCESS;
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    returnCode = runGlobalMaster();
  } else {
    runWorker();
  }

  shutdownEnvironment();
  return returnCode;
}
