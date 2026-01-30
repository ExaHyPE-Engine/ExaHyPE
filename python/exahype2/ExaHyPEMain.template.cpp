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

  static double printNextTimeStep = 0.0;
  static bool haveReceivedNonCriticalAssertion = false;

  template <typename T>
  std::string toScientificNotationString(T value, int precision = 4, bool considerSign = false, double threshold = 1e6) {
    std::ostringstream out;
    double v = static_cast<double>(value);

    if (std::abs(v) < threshold) {
      if (considerSign) {
        out << std::showpos;
      }
      out << value;
      return out.str();
    }

    out << std::scientific << std::setprecision(precision);
    if (considerSign) {
      out << std::showpos;
    }
    out << v;
    return out.str();
  }
} // namespace

/**
 * Evaluates the state of the mesh and decides if construction/refinement loops should continue.
 *
 * @return true if construction should continue (sets next step implicitly), false if construction is done.
 */
bool shouldContinueGridConstruction(double& currentMinH, int& currentGlobalNumberOfTrees) {
  assertion(repositories::loadBalancer.getGlobalNumberOfTrees() > 0);

  // If the mesh size has decreased, we pause refinement to let the grid settle.
  if (tarch::la::smaller(repositories::loadBalancer.getGlobalMinH(), currentMinH)) {
    [[maybe_unused]] const double oldH = currentMinH; // Only for logging
    currentMinH = repositories::loadBalancer.getGlobalMinH();

    logDebug(
      "shouldContinueGridConstruction()",
      "Mesh refinement detected (h_{min} decreased: " << oldH << " -> " << currentMinH << "). " <<
      "Switching to 'CreateGridWithoutRefinement' to allow grid to settle."
    );

    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  // If new trees (subdomains) have appeared, we pause refinement to let the load balancer settle.
  if (repositories::loadBalancer.getGlobalNumberOfTrees() > currentGlobalNumberOfTrees) {
    const int newTreeCount = repositories::loadBalancer.getGlobalNumberOfTrees();

    logDebug(
      "shouldContinueGridConstruction()",
      "Rebalancing detected (Tree count increased: " << currentGlobalNumberOfTrees << " -> " << newTreeCount << "). " <<
      "Switching to 'CreateGridWithoutRefinement' to postpone further refinement."
    );

    currentGlobalNumberOfTrees = newTreeCount;
    peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithoutRefinement));
    return true;
  }

  // If the grid is stable (small enough and converged), terminate.
  if (
    tarch::la::smallerEquals(currentMinH, repositories::getMinAdmissibleMeshSize())
    and
    repositories::loadBalancer.hasStagnated()
    and
    // Ensure that a proper creation has been ran before, so the mesh had the opportunity to refine further if it has not done yet.
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()) == repositories::StepRepository::Steps::CreateGridWithRefinement
  ) {
    // If we get in an infinite loop because Peano is not able to get all ranks stationary, then we break the loop after reaching the number of settling iterations given by the load balancer.
    // Happens if we have small meshes, and too many ranks to satisfy the mesh splitting.
    static int numberOfStationarySweeps = 0;
    if (repositories::loadBalancer.getGlobalNumberOfStationarySweeps() == 0 and numberOfStationarySweeps < repositories::loadBalancer.getNumberOfSettlingIterations()) {
      logDebug(
        "shouldContinueGridConstruction()",
        "Admissible mesh size reached, but at least one rank still has pending refinement events."
      );
      peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithRefinement));
      ++numberOfStationarySweeps;
      return true;
    }

    logDebug(
      "shouldContinueGridConstruction()",
      "Grid construction complete. Admissible mesh size reached (h_{min}=" << currentMinH << ", admissible h_{min}=" << repositories::getMinAdmissibleMeshSize() << ") and load balancing has converged."
    );
    return false;
  }

  // If we are here, the mesh is stationary but not yet fine enough.
  peano4::parallel::Node::getInstance().setNextProgramStep(repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGridWithRefinement));
  return true;
}

/**
 * Decide which step to run next
 *
 * ## Control of the parallel grid construction
 *
 * @return continues to run
 */
bool selectNextAlgorithmicStep() {
  static bool                                   gridConstructed                       = false;
  static bool                                   gridInitialised                       = false;
  static double                                 nextMaxPlotTimeStamp                  = FirstPlotTimeStamp;
  static double                                 nextMinPlotTimeStamp                  = FirstPlotTimeStamp;
  static bool                                   haveJustWrittenSnapshot               = false;
  static double                                 minH                                  = std::numeric_limits<double>::max();
  static int                                    globalNumberOfTrees                   = 0;
  bool                                          continueToSolve                       = true;

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

void printNextAlgorithmicStep(const repositories::StepRepository::Steps& stepName, bool printStatistics) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("printNextAlgorithmicStep()", "Starting AlgorithmicStep " << "[" << repositories::StepRepository::toString(stepName) << "]");

    if (printStatistics) {
      const auto minLoad                = repositories::loadBalancer.getGlobalMinTreeLoad();
      const auto maxLoad                = repositories::loadBalancer.getGlobalMaxTreeLoad();
      const auto loadPct                = repositories::loadBalancer.getGlobalTreeLoadRatioInPercentage();

      const auto memoryUsedByGrid       = repositories::loadBalancer.getGlobalMemoryUsageInBytes();
      const auto memoryUsedByHeap       = repositories::DataRepository::getGlobalMemoryUsageInBytes();
      const auto totalPersistentMemory  = memoryUsedByGrid + memoryUsedByHeap;

      const auto dofs                   = static_cast<double>(repositories::getNumberOfDoFs());
      const auto globalNumberOfPatches  = static_cast<double>(repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells());
      const auto totalNumberOfDoFs      = dofs * globalNumberOfPatches;

      std::ostringstream msg;
      // TODO: Write everything in a log file as well
      msg << "#Patches="  << toScientificNotationString(globalNumberOfPatches)
          << ", #DoFs="   << toScientificNotationString(totalNumberOfDoFs)
          << ", #Trees="  << toScientificNotationString(repositories::loadBalancer.getGlobalNumberOfTrees())
          << ", #Load="   << std::fixed << std::setprecision(2) << loadPct << "% (min=" << minLoad << ", max=" << maxLoad << ")"
          << ", #Memory=" << std::fixed << std::setprecision(2) << tarch::convertMemorySize(totalPersistentMemory, tarch::chooseMemoryUsageFormat(totalPersistentMemory)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(totalPersistentMemory))
          << " (Grid="    << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByGrid, tarch::chooseMemoryUsageFormat(memoryUsedByGrid)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByGrid))
          << ", Heap="    << std::fixed << std::setprecision(2) << tarch::convertMemorySize(memoryUsedByHeap, tarch::chooseMemoryUsageFormat(memoryUsedByHeap)) << " " << tarch::toString(tarch::chooseMemoryUsageFormat(memoryUsedByHeap))
          << ")";
      logInfo("printNextAlgorithmicStep()", msg.str());

      tarch::logging::Statistics::getInstance().log("MemoryUsage", tarch::accelerator::MemoryManager::getStatistics());
      tarch::logging::Statistics::getInstance().log("LoadBalancing", toolbox::loadbalancing::LoadBalancer::getStatistics());

      if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
        const auto dt               = timeStepMeasurement.getMostRecentValue();
        const auto patchesPerSecond = globalNumberOfPatches / dt;
        const auto dofsPerSecond    = totalNumberOfDoFs     / dt;
        logInfo("printNextAlgorithmicStep()",
          "Patch updates per second: " << toScientificNotationString(patchesPerSecond) <<
          ", DoF updates per second: " << toScientificNotationString(dofsPerSecond)
        );
      }
    }
  }
}

/**
 * Perform one algorithmic step
 */
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

  repositories::DataRepository::updateMemoryUsage();
}

void runNextAlgorithmicStep() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  switch (stepName) {
  case repositories::StepRepository::Steps::CreateGridWithoutRefinement:
    printNextAlgorithmicStep(stepName, true);
    performAlgorithmicStep<observers::CreateGridWithoutRefinement>(
      stepName,
      [](){ repositories::startGridConstructionStep(); },
      [](){ repositories::finishGridConstructionStep(); },
      gridConstructionMeasurement
    );
    break;
  case repositories::StepRepository::Steps::CreateGridWithRefinement:
    printNextAlgorithmicStep(stepName, false);
    performAlgorithmicStep<observers::CreateGridWithRefinement>(
      stepName,
      [](){ repositories::startGridConstructionStep(); },
      [](){ repositories::finishGridConstructionStep(); },
      gridConstructionMeasurement
    );
    break;
  case repositories::StepRepository::Steps::InitGrid:
    printNextAlgorithmicStep(stepName, true);
    repositories::loadBalancer.enable(false); // TODO: Remove (dynamic AMR)
    performAlgorithmicStep<observers::InitGrid>(
      stepName,
      [](){ repositories::startGridInitialisationStep(); },
      [](){ repositories::finishGridInitialisationStep(); },
      gridInitialisationMeasurement
    );
    break;
  case repositories::StepRepository::Steps::PlotSolution:
    printNextAlgorithmicStep(stepName, false);
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
    if (timeStepMeasurement.getAccumulatedValue() >= printNextTimeStep and repositories::mayPlot()) {
      printNextAlgorithmicStep(stepName, true);
      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        repositories::printTimeStep(false);
      }
      printNextTimeStep += repositories::PrintTimeStepIntervalInSeconds;
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

void printFinalStatistics() {
  if (gridConstructionMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("printFinalStatistics()", "Grid construction:                 " << gridConstructionMeasurement.getAccumulatedValue() << "s\t" << gridConstructionMeasurement.toString());
  }
  if (gridInitialisationMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("printFinalStatistics()", "Grid initialisation:               " << gridInitialisationMeasurement.getAccumulatedValue() << "s\t" << gridInitialisationMeasurement.toString());
  }
  if (plotMeasurement.getNumberOfMeasurements() > 0) {
    logInfo("printFinalStatistics()", "Plotting:                          " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString());
  }
  if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
    const auto dofs                   = static_cast<double>(repositories::getNumberOfDoFs());
    const auto globalNumberOfPatches  = static_cast<double>(repositories::loadBalancer.getGlobalNumberOfInnerUnrefinedCells());
    const auto totalNumberOfDoFs      = dofs * globalNumberOfPatches;
    const auto dt                     = timeStepMeasurement.getValue();
    const auto patchesPerSecond       = globalNumberOfPatches / dt;
    const auto dofsPerSecond          = totalNumberOfDoFs     / dt;
    logInfo("printFinalStatistics()", "Time stepping:                     " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString());
    logInfo("printFinalStatistics()", "Average patch updates per second:  " << toScientificNotationString(patchesPerSecond));
    logInfo("printFinalStatistics()", "Average DoF updates per second:    " << toScientificNotationString(dofsPerSecond));
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
    repositories::printTimeStep(false);
    logInfo("runGlobalMaster()", "Terminated successfully");
    printFinalStatistics();
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
