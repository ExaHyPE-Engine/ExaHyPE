#include "{{MAIN_NAME}}.h"

#include "observers/Assemble.h"
#include "observers/CreateGrid.h"
#include "observers/EnumerateAndInitSolution.h"
#include "observers/MapSolutionOntoMesh.h"
#include "observers/Plot.h"
#include "observers/Solve.h"

#include "peano4/peano.h"
#include "peano4/UnitTests.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "petsc/petsc.h"
#include "petsc/UnitTests.h"

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

// #include "petsc/UserInterface.h"

{% if FENV_ARGS is defined and FENV_ARGS != "" -%}
#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON
{% endif -%}

using namespace {{NAMESPACE | join("::")}};


tarch::logging::Log _log("::");


tarch::timing::Measurement createGridMeasurement;
tarch::timing::Measurement enumerateMeasurement;
tarch::timing::Measurement initMeasurement;
tarch::timing::Measurement assembleMeasurement;
tarch::timing::Measurement solveMeasurement;
tarch::timing::Measurement mapSolutionOntoMeshMeasurement;
tarch::timing::Measurement plotMeasurement;


/**
 *
 * <h2> Control of the parallel grid construction </h2>
 *
 *
 *
 * @return continues to run
 */
bool selectNextAlgorithmicStep() {
  static bool gridCreated = false;

  bool continueToSolve = true;

  if (gridCreated) {
    int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
    auto stepName       = repositories::StepRepository::toStepEnum(stepIdentifier);

    switch ( stepName ) {
      case repositories::StepRepository::Steps::CreateGrid:
        {
            peano4::parallel::Node::getInstance().setNextProgramStep(
              repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::EnumerateAndInitSolution )
            );
        }
        break;
     
      case repositories::StepRepository::Steps::EnumerateAndInitSolution:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::InitPETSc )
          );
        }
        break;
      case repositories::StepRepository::Steps::InitPETSc:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Assemble )
          );
        }
        break;
      case repositories::StepRepository::Steps::Assemble:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Solve )
          );
        }
        break;
      case repositories::StepRepository::Steps::Solve:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::MapSolutionOntoMesh )
          );
        }
        break;
      case repositories::StepRepository::Steps::MapSolutionOntoMesh:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
          );
        }
        break;
      case repositories::StepRepository::Steps::Plot:
        {
          continueToSolve = false;
        }
        break;
      case repositories::StepRepository::Steps::Undef:
        assertion(false);
        break;
    }
  } else {
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
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        createGridMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    
    case repositories::StepRepository::Steps::EnumerateAndInitSolution:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "enumerate-and-init-solution" );

        observers::EnumerateAndInitSolution  observer;
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
        enumerateMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::InitPETSc:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "init-PETSc" );

        watch.start();

        repositories::computeLocalToGlobalMapsForAllSolvers();
        repositories::initMatricesAndVectors();
        logInfo( "step()", "PETSc successfully initialised" );
        
        watch.stop();
        initMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Assemble:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "assemble" );

        observers::Assemble  observer;
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);

        watch.stop();
        assembleMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Solve:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "solve" );

        watch.start();
        repositories::solve();
        watch.stop();
        
        solveMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::MapSolutionOntoMesh:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "map-solution-onto-mesh" );

        observers::MapSolutionOntoMesh  observer;
        watch.start();

        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        
        watch.stop();
        mapSolutionOntoMeshMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Plot:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot" );

        observers::Plot  observer;
        watch.start();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        watch.stop();
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
  petsc::initParallelEnvironment(&argc, &argv);
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
      "main()", "no petsc.log-filter file found or file has been corrupted. Use default logging configuration"
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
  unitTests->addTestCase(petsc::getUnitTests());
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
          "enumerate:                   " << enumerateMeasurement.getAccumulatedValue(
          ) << "s\t" << enumerateMeasurement.toString()
        );
        logInfo(
          "main()",
          "init (setup PETSc):          " << initMeasurement.getAccumulatedValue(
          ) << "s\t" << initMeasurement.toString()
        );
        logInfo(
          "main()",
          "assemble:                    " << assembleMeasurement.getAccumulatedValue(
          ) << "s\t" << assembleMeasurement.toString()
        );
        logInfo(
          "main()",
          "solve:                       " << solveMeasurement.getAccumulatedValue(
          ) << "s\t" << solveMeasurement.toString()
        );
        logInfo(
          "main()",
          "map solution back onto mesh: " << mapSolutionOntoMeshMeasurement.getAccumulatedValue(
          ) << "s\t" << mapSolutionOntoMeshMeasurement.toString()
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

  // repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().cleanUpPetscObjects();

  peano4::shutdownSingletons();
  repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
