// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{MAIN_NAME}}.h"

#include "Constants.h"
#include "RuntimeParameters.h"
#include "observers/CreateGridWithRefinement.h"
#include "observers/CreateGridWithoutRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"
#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

#include "tarch/tarch.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/instrumentation/Watch.h"

#include "peano4/peano4.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "exahype2/RuntimeParameters.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

using namespace {{NAMESPACE | join("::")}};

namespace {
  static tarch::instrumentation::Watch watch("::", "", false, false);

  tarch::instrumentation::Measurement gridConstructionMeasurement;
  tarch::instrumentation::Measurement gridInitialisationMeasurement;
  tarch::instrumentation::Measurement timeStepMeasurement;
  tarch::instrumentation::Measurement plotMeasurement;
  tarch::instrumentation::Measurement gridSweepMeasurement;

  bool helpRequested(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
      const std::string argument = argv[index];
      if (argument == "--help" or argument == "-h") {
        return true;
      }
    }
    return false;
  }

  static double logNextTimeStep = 0.0;
  static int currentNumberOfGridSweepsOfTimeStep = 0;
  static unsigned long numberOfCompletedTimeSteps = 0;
  static unsigned long lastLoggedNumberOfCompletedTimeSteps = 0;
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
  const double minAdmissiblePatchSize = ::exahype2::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);

  // If the mesh size has decreased, we pause refinement to let the grid settle.
  if (tarch::la::smaller(repositories::loadBalancer.getStatistics().getGlobalMinH(), currentMinH)) {
    [[maybe_unused]] const double oldH  = currentMinH; // Only for logging
    currentMinH                         = repositories::loadBalancer.getStatistics().getGlobalMinH();
    finishConditionSeenLastSweep        = false;
    const auto globalNumberOfLeaves     = static_cast<unsigned long>(repositories::loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto maxDepth                 = repositories::loadBalancer.getStatistics().getGlobalMaxDepth();
    logInfo("Mesh refinement detected (h_{min} decreased: " << oldH << " -> " << currentMinH << "), Leaves=" << globalNumberOfLeaves << ", Levels=" << maxDepth);
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  // If new trees (subdomains) have appeared, we pause refinement to let the load balancer settle.
  if (repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees() > currentGlobalNumberOfTrees) {
    const auto newTreeCount = repositories::loadBalancer.getStatistics().getGlobalNumberOfTrees();
    logInfo("Rebalancing detected (Tree count increased: " << currentGlobalNumberOfTrees << " -> " << newTreeCount << ")");
    currentGlobalNumberOfTrees = newTreeCount;
    finishConditionSeenLastSweep = false;
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  const bool hasReachedGridConstructionConvergence =
    tarch::la::smallerEquals(currentMinH, minAdmissiblePatchSize)
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
      logInfo("Grid construction complete: Admissible mesh size reached (h_{min}=" << currentMinH << ", admissible h_{min}=" << minAdmissiblePatchSize << ") and load balancing has converged");
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
  static double  nextMaxPlotTimeStamp    = ::exahype2::RuntimeParameters::getInstance().getFirstPlotTimeStamp();
  static double  nextMinPlotTimeStamp    = ::exahype2::RuntimeParameters::getInstance().getFirstPlotTimeStamp();
  static bool    haveJustWrittenSnapshot = false;
  static double  minH                    = std::numeric_limits<double>::max();
  static int     globalNumberOfTrees     = 0;
  bool           continueToSolve         = true;
  const auto&    runtimeParameters       = ::exahype2::RuntimeParameters::getInstance();
  const double   minTerminalTime         = runtimeParameters.getMinTerminalTime();
  const double   maxTerminalTime         = runtimeParameters.getMaxTerminalTime();
  const double   plottingEndTime         = maxTerminalTime < std::numeric_limits<double>::max() / 10.0 ? maxTerminalTime : minTerminalTime;
  const int      numberOfSnapshots       = runtimeParameters.getNumberOfSnapshots();
  const double   snapshotInterval        = numberOfSnapshots <= 0 ? 0.0 : plottingEndTime / numberOfSnapshots;

  if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
    haveReceivedNonCriticalAssertion = true;
    logError("Noncritical assertion has been triggered in code. Dump final state and terminate.");
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
      snapshotInterval > 0.0
      and
      repositories::getMinTimeStamp() < minTerminalTime
      and
      repositories::getMaxTimeStamp() < maxTerminalTime
      and
      (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
      and
      repositories::mayPlot()
    ) {
      nextMinPlotTimeStamp += snapshotInterval;
      nextMaxPlotTimeStamp += snapshotInterval;

      if (nextMinPlotTimeStamp < repositories::getMinTimeStamp()) {
        logWarning("Code is asked to plot every dt="
            << snapshotInterval << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << (repositories::getMinTimeStamp() + snapshotInterval));
        nextMinPlotTimeStamp = repositories::getMinTimeStamp() + snapshotInterval;
      } else if (nextMaxPlotTimeStamp < repositories::getMaxTimeStamp()) {
        logWarning("Code is asked to plot every dt="
            << snapshotInterval << ", but this seems to be less than the time step size of the solvers. "
            << "So postpone next plot to t=" << (repositories::getMaxTimeStamp() + snapshotInterval));
        nextMaxPlotTimeStamp = repositories::getMaxTimeStamp() + snapshotInterval;
      }

      nextMaxPlotTimeStamp = std::max(nextMaxPlotTimeStamp, nextMinPlotTimeStamp);

      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
      haveJustWrittenSnapshot = true;
    } else if (repositories::getMinTimeStamp() < minTerminalTime and repositories::getMaxTimeStamp() < maxTerminalTime) {
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::TimeStep));
      continueToSolve         = true;
      haveJustWrittenSnapshot = false;
    } else {
      if (not haveJustWrittenSnapshot and snapshotInterval > 0.0 and repositories::mayPlot()) {
        peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::PlotSolution));
        haveJustWrittenSnapshot = true;
        nextMinPlotTimeStamp    = std::numeric_limits<double>::max();
        nextMaxPlotTimeStamp    = std::numeric_limits<double>::max();
      } else if (not haveJustWrittenSnapshot and snapshotInterval > 0.0 and not repositories::mayPlot()) {
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
    logInfo("Topology" << tarch::align << msg.str());
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
    logInfo("Load Balancing" << tarch::align << msg.str());
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
    logInfo("Memory Usage" << tarch::align << msg.str());
  }

  if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
    const auto dofs               = static_cast<unsigned long>(repositories::getNumberOfDoFs());
    const auto totalNumberOfDoFs  = dofs * globalNumberOfLeaves;
    const auto dt                 = timeStepMeasurement.getMostRecentValue();
    const auto leavesPerSecond    = globalNumberOfLeaves / dt;
    const auto dofsPerSecond      = totalNumberOfDoFs    / dt;
    std::ostringstream throughputMessage;
    throughputMessage << "Throughput per Time Step" << tarch::align << leavesPerSecond << " Leaf/s, " << dofsPerSecond << " DoF/s";
    if (gridSweepMeasurement.getNumberOfMeasurements() > 0) {
      const auto gridSweepsPerTimestep = gridSweepMeasurement.getMostRecentValue();
      const auto gridSweepsPerSecond   = gridSweepsPerTimestep / dt;
      throughputMessage << ", " << gridSweepsPerSecond << " Sweep/s, " << gridSweepsPerTimestep << " Sweep/Time Step";
    }
    logInfo(throughputMessage.str());

    const auto elapsedWallTime = timeStepMeasurement.getAccumulatedValue() + plotMeasurement.getAccumulatedValue();
    const auto minTimeStamp    = repositories::getMinTimeStamp();
    const auto maxTimeStamp    = repositories::getMaxTimeStamp();
    const auto minTerminalTime = ::exahype2::RuntimeParameters::getInstance().getMinTerminalTime();
    const auto maxTerminalTime = ::exahype2::RuntimeParameters::getInstance().getMaxTerminalTime();

    double expectedTimeUntilFinish = std::numeric_limits<double>::max();
    if (elapsedWallTime > 0.0 and std::isnormal(minTimeStamp) and minTimeStamp > 10.0 * std::numeric_limits<double>::min() and minTimeStamp < minTerminalTime) {
      expectedTimeUntilFinish = std::min(expectedTimeUntilFinish, (minTerminalTime - minTimeStamp) * elapsedWallTime / minTimeStamp);
    }
    if (elapsedWallTime > 0.0 and std::isnormal(maxTimeStamp) and maxTimeStamp > 10.0 * std::numeric_limits<double>::min() and maxTimeStamp < maxTerminalTime) {
      expectedTimeUntilFinish = std::min(expectedTimeUntilFinish, (maxTerminalTime - maxTimeStamp) * elapsedWallTime / maxTimeStamp);
    }
    if (expectedTimeUntilFinish < std::numeric_limits<double>::max() / 10.0) {
      const auto expectedTimeUntilFinishInSeconds = static_cast<int>(expectedTimeUntilFinish + 0.5);
      const auto hours   = expectedTimeUntilFinishInSeconds / 3600;
      const auto minutes = (expectedTimeUntilFinishInSeconds % 3600) / 60;
      const auto seconds = expectedTimeUntilFinishInSeconds % 60;
      logInfo("Time to Completion" << tarch::align << hours << "h " << minutes << "m " << seconds << "s");
    }
  }
}

template <class Observer>
void performAlgorithmicStep(
  repositories::StepRepository::Steps stepName,
  std::function<void()>               startStepHook,
  std::function<void()>               finishStepHook
) {
  tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);

  startStepHook();
  Observer::prepareTraversal();

  Observer observer;
  peano4::parallel::SpacetreeSet::getInstance().traverse(observer);

  Observer::unprepareTraversal();
  finishStepHook();
}

void runNextAlgorithmicStep() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  switch (stepName) {
  case repositories::StepRepository::Steps::CreateGridWithoutRefinement:
    performAlgorithmicStep<observers::CreateGridWithoutRefinement>(
      stepName,
      [](){
        repositories::startGridConstructionStep();
        watch.start();
      },
      [](){
        watch.stop();
        gridConstructionMeasurement.setValue(watch.getCalendarTime());
        repositories::finishGridConstructionStep();
      }
    );
    break;
  case repositories::StepRepository::Steps::CreateGridWithRefinement:
    performAlgorithmicStep<observers::CreateGridWithRefinement>(
      stepName,
      [](){
        repositories::startGridConstructionStep();
        watch.start();
      },
      [](){
        watch.stop();
        gridConstructionMeasurement.setValue(watch.getCalendarTime());
        repositories::finishGridConstructionStep();
      }
    );
    break;
  case repositories::StepRepository::Steps::InitGrid:
    repositories::loadBalancer.enable(false); // TODO: Remove (dynamic AMR)
    performAlgorithmicStep<observers::InitGrid>(
      stepName,
      [](){
        repositories::startGridInitialisationStep();
        watch.start();
      },
      [](){
        watch.stop();
        gridInitialisationMeasurement.setValue(watch.getCalendarTime());
        repositories::finishGridInitialisationStep();
      }
    );
    logInfo("Grid initialisation complete");
    break;
  case repositories::StepRepository::Steps::PlotSolution:
    performAlgorithmicStep<observers::PlotSolution>(
      stepName,
      [](){
        repositories::startPlottingStep();
        watch.start();
      },
      [](){
        watch.stop();
        plotMeasurement.setValue(watch.getCalendarTime());
        repositories::finishPlottingStep();
      }
    );
    break;
  case repositories::StepRepository::Steps::TimeStep:
    performAlgorithmicStep<observers::TimeStep>(
      stepName,
      [](){
        repositories::startTimeStep();
        if (currentNumberOfGridSweepsOfTimeStep == 0) {
          watch.start();
        }
        currentNumberOfGridSweepsOfTimeStep++;
      },
      [](){
        repositories::finishTimeStep();
        if (repositories::mayPlot()) {
          watch.stop();
          timeStepMeasurement.setValue(watch.getCalendarTime());
          gridSweepMeasurement.setValue(static_cast<double>(currentNumberOfGridSweepsOfTimeStep));
          currentNumberOfGridSweepsOfTimeStep = 0;
          numberOfCompletedTimeSteps++;
        }
      }
    );
    if (
      not tarch::hasNonCriticalAssertionBeenViolated()
      and
      timeStepMeasurement.getAccumulatedValue() >= logNextTimeStep
      and
      repositories::mayPlot()
    ) {
      repositories::logTimeStep();
      logGlobalStatistics();
      lastLoggedNumberOfCompletedTimeSteps = numberOfCompletedTimeSteps;
      logNextTimeStep += ::exahype2::RuntimeParameters::getInstance().getLogIntervalInSeconds();
    }
    break;
  case repositories::StepRepository::Steps::Undef:
    assertion(false);
    break;
  }
}

void setupEnvironment(int argc, char** argv) {
  auto& runtimeParameters = ::exahype2::RuntimeParameters::getInstance();
  runtimeParameters.setGeneratedDefaults(
    {{DEFAULT_MIN_TERMINAL_TIME}},
    {{DEFAULT_MAX_TERMINAL_TIME}},
    {{DEFAULT_GRID_LEVELS}},
    {{DEFAULT_MAX_GRID_LEVELS}}
  );
  if (helpRequested(argc, argv)) {
    runtimeParameters.printUsage({{EXECUTABLE_NAME}});
    RuntimeParameters::printUsage();
    std::exit(EXIT_SUCCESS);
  }
  runtimeParameters.configure(argc, argv);
  RuntimeParameters::load(runtimeParameters.getConfigFilePath(), argc, argv);

  peano4::init(&argc, &argv, DomainOffset, DomainSize, PeriodicBC);

  if (not runtimeParameters.getConfigFilePath().empty()) {
    logInfo("Loaded runtime configuration from " << runtimeParameters.getConfigFilePath());
  }

  const int numberOfMaxGlobalSpacetrees = repositories::loadBalancer.getMaxPermittedTreesPerRank() * tarch::mpi::Rank::getInstance().getNumberOfRanks();
  repositories::DataRepository::setNumberOfMaxGlobalSpacetrees(numberOfMaxGlobalSpacetrees);
  repositories::DataRepository::init();
  repositories::startSimulation();
}

void shutdownEnvironment() {
  repositories::finishSimulation();
  repositories::DataRepository::shutdown();

  peano4::shutdown();
}

void logFinalGlobalStatistics() {
  if (gridConstructionMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("Grid Construction" << tarch::align << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString());
  }
  if (gridInitialisationMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("Grid Initialisation" << tarch::align << gridInitialisationMeasurement.getAccumulatedValue() << "s\t" << gridInitialisationMeasurement.toString());
  }
  if (plotMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("Plotting" << tarch::align << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString());
  }
  if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
    const auto dofs                   = static_cast<double>(repositories::getNumberOfDoFs());
    const auto globalNumberOfLeaves   = static_cast<double>(repositories::loadBalancer.getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto totalNumberOfDoFs      = dofs * globalNumberOfLeaves;
    const auto dt                     = timeStepMeasurement.getValue();
    const auto leavesPerSecond        = globalNumberOfLeaves / dt;
    const auto dofsPerSecond          = totalNumberOfDoFs    / dt;
    logInfo("Time Stepping" << tarch::align << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString());
    std::ostringstream throughputMessage;
    throughputMessage << "Average Throughput per Time Step" << tarch::align << leavesPerSecond << " Leaf/s, " << dofsPerSecond << " DoF/s";
    if (gridSweepMeasurement.getNumberOfMeasurements() > 0) {
      const auto gridSweepsPerTimestep = gridSweepMeasurement.getValue();
      const auto gridSweepsPerSecond   = gridSweepsPerTimestep / dt;
      throughputMessage << ", " << gridSweepsPerSecond << " Sweep/s, Sweep/Time Step " << gridSweepMeasurement.toString();
    }
    logInfo(throughputMessage.str());
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
    logWarning("ExaHyPE terminated gracefully after noncritical assertion");
    return EXIT_FAILURE;
  } else {
    if (numberOfCompletedTimeSteps > lastLoggedNumberOfCompletedTimeSteps) {
      repositories::logTimeStep();
      lastLoggedNumberOfCompletedTimeSteps = numberOfCompletedTimeSteps;
    }
    logInfo("ExaHyPE terminated successfully");
    logFinalGlobalStatistics();
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

#if defined(WITH_OPENMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        returnCode = runGlobalMaster();
      } else {
        runWorker();
      }
#if defined(WITH_OPENMP)
    }
  }
#endif

  shutdownEnvironment();
  return returnCode;
}
