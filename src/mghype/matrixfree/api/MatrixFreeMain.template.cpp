#include "{{MAIN_NAME}}.h"

#include "observers/CreateGrid.h"
#include "observers/InitSolution.h"
#include "observers/Solve.h"
#include "observers/Plot.h"

#include "peano4/peano.h"
#include "peano4/UnitTests.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"

#include "tarch/UnitTests.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/tests/TreeTestCaseCollection.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

#include "toolbox/finiteelements/UnitTests.h"
#include "toolbox/loadbalancing/loadbalancing.h"

{% if FENV_ARGS is defined and FENV_ARGS != "" -%}
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
{% endif -%}

using namespace {{NAMESPACE | join("::")}};


tarch::logging::Log _log("::");

tarch::timing::Measurement createGridMeasurement;
tarch::timing::Measurement initMeasurement;
tarch::timing::Measurement solveMeasurement;
tarch::timing::Measurement plotMeasurement;


bool selectNextAlgorithmicStep() {
  static bool gridCreated = false;
  
  static int solverSteps;

  bool continueToSolve = true;

  static bool plotThenExit = false;

  const int MaxIterations = 5000;

  if (gridCreated){
    int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
    auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

    switch ( stepName ) {
      case repositories::StepRepository::Steps::CreateGrid:
        {
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::InitSolution )
            );
        }
        break;
     
      case repositories::StepRepository::Steps::InitSolution:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
          );
        }
        break;

{% if PlotEachTimeStep %}

      case repositories::StepRepository::Steps::Solve:
        {
          {
          if (
            ++solverSteps >= MaxIterations
            or
            repositories::terminationCriterionHolds()
          ) plotThenExit = true;
          
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
          );
          }
        } break;

      case repositories::StepRepository::Steps::Plot:
        {
          if (plotThenExit) {
            continueToSolve = false;
            logInfo("selectNextAlgorithmicStep()", "terminating after " << solverSteps << " steps");
          }

          else 
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Solve )
            );
        } break;

{% else  %}

      case repositories::StepRepository::Steps::Solve:
        {
          if (
            ++solverSteps >= MaxIterations
            or
            repositories::terminationCriterionHolds()
          )
          {
            plotThenExit = true;
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
            );
          }
          else
          {
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Solve )
            );
          }
        }
        break;
      case repositories::StepRepository::Steps::Plot:
        {
          if ( plotThenExit ) {
            logInfo("selectNextAlgorithmicStep()", "terminating after " << solverSteps << " steps");
            continueToSolve = false;
          }
          else 
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Solve )
            );
        }
        break;

{% endif %}
    }
  }
  else {
    static tarch::la::Vector<Dimensions, double> minH = tarch::la::Vector<Dimensions, double>(
      std::numeric_limits<double>::max()
    );
    static int globalNumberOfTrees = 0;

    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::CreateGrid)
    );

    if (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps() > 5) {
      logInfo(
        "selectNextAlgorithmicStep()", "mesh has been stationary for more than 5 grid sweeps. Stop grid construction"
      );
      gridCreated = true;
    }
  }
  return continueToSolve;
}

void step() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

  static tarch::logging::Log _log("");
#if PeanoDebug > 0
#else
  if (tarch::mpi::Rank::getInstance().isGlobalMaster())
#endif
  logInfo("step()", "run " << repositories::StepRepository::toString(stepName));

  static tarch::timing::Watch watch("::", "step()", false);

  static int creepingNumberOfLocalCells = 0;

  switch ( stepName ) {
    case repositories::StepRepository::Steps::CreateGrid:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "create-grid" );

        observers::CreateGrid  observer;
        observers::CreateGrid::prepareTraversal();
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        observers::CreateGrid::unprepareTraversal();
        createGridMeasurement.setValue( watch.getCalendarTime() );
      }
      break;

    case repositories::StepRepository::Steps::InitSolution:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "init-solution" );

        observers::InitSolution  observer;
        observers::InitSolution::prepareTraversal();
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        observers::InitSolution::unprepareTraversal();
        initMeasurement.setValue( watch.getCalendarTime() );
      }
      break;

    case repositories::StepRepository::Steps::Solve:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "solve" );
        
        observers::Solve observer;
        observers::Solve::prepareTraversal();
        watch.start();
        repositories::beginMeshSweep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::endMeshSweep();
        watch.stop();
        observers::Solve::unprepareTraversal();

        solveMeasurement.setValue( watch.getCalendarTime() );
      }
      break;

    case repositories::StepRepository::Steps::Plot:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot" );

        observers::Plot  observer;
        observers::Plot::prepareTraversal();
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        observers::Plot::unprepareTraversal();
        plotMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Undef:
      assertion(false);
      break;
  }
}



int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeUnitTestsFailed  = 1;
  const int ExitCodeInvalidArguments = 2;
  const int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc, &argv);
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

  tarch::la::Vector<Dimensions, double> x = {{DomainOffset}};
  tarch::la::Vector<Dimensions, double> h = {{DomainSize}};

  peano4::initSingletons(x, h);

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and tarch::multicore::Core::getInstance().getNumberOfThreads() <= 1) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not tarch::logging::LogFilterFileReader::parsePlainTextFile("multigrid.log-filter")) {
    logWarning(
      "main()", "no multigrid.log-filter file found or file has been corrupted. Use default logging configuration"
    );
  }

  repositories::DataRepository::initDatatypes();

  {% if FENV_ARGS is defined and FENV_ARGS != "" -%}
  feenableexcept({{FENV_ARGS}} );
  {% endif -%}

#if PeanoDebug >= 2
  tarch::tests::TreeTestCaseCollection* unitTests = new tarch::tests::TreeTestCaseCollection();
  unitTests->addTestCase(peano4::getUnitTests());
  unitTests->addTestCase(tarch::getUnitTests());
  unitTests->addTestCase(toolbox::finiteelements::getUnitTests());
  unitTests->run();
  if (unitTests->getNumberOfErrors() != 0) {
    logError("main()", "unit tests failed. Quit.");
    tarch::mpi::Rank::abort(ExitCodeUnitTestsFailed);
  }
  delete unitTests;
#endif

  tarch::logging::Statistics::getInstance().clear();

  OTTER_INITIALISE();

#if defined(SharedOMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif

#if defined(UseSmartMPI)
      const bool isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster() and smartmpi::isComputeRank();
      const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster() and smartmpi::isComputeRank();
#else
  const bool isGlobalMaster     = tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();
#endif

      if (isGlobalMaster) {
        while (selectNextAlgorithmicStep()) {
          step();
        }

        logInfo("main()", "terminated successfully");
        logInfo(
          "main()",
          "grid construction:           " << createGridMeasurement.getAccumulatedValue(
          ) << "s\t" << createGridMeasurement.toString()
        );
        logInfo(
          "main()",
          "init:                        " << initMeasurement.getAccumulatedValue(
          ) << "s\t" << initMeasurement.toString()
        );
        logInfo(
          "main()",
          "solve:                       " << solveMeasurement.getAccumulatedValue(
          ) << "s\t" << solveMeasurement.toString()
        );
        logInfo(
          "main()",
          "plotting:                    " << plotMeasurement.getAccumulatedValue(
          ) << "s\t" << plotMeasurement.toString()
        );
      } else if (isPeanoComputeNode) {
        while (peano4::parallel::Node::getInstance().continueToRun()) {
          step();
        }
      }
#if defined(UseSmartMPI)
      else {
        while (smartmpi::continueToRun()) {
          smartmpi::tick();
        }
      }
#endif

#if defined(SharedOMP)
    }
  }
#endif

  tarch::logging::Statistics::getInstance().writeToCSV();

  OTTER_FINALISE();


  peano4::shutdownSingletons();
  repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}

