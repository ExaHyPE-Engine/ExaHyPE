#include "{{MAIN_NAME}}.h"

#include "observers/CreateGrid.h"
#include "observers/EnumerateAndInitSolution.h"
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
tarch::timing::Measurement enumerateMeasurement;
tarch::timing::Measurement initMeasurement;
tarch::timing::Measurement solveMeasurement;
tarch::timing::Measurement plotMeasurement;



bool selectNextAlgorithmicStep() {
  static bool gridCreated = false;

  bool continueToSolve = true;

  if (gridCreated){
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
      case repositories::StepRepository::Steps::Solve:
        {
          peano4::parallel::Node::getInstance().setNextProgramStep(
            repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::MapSolutionOntoMesh )
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
    
    case repositories::StepRepository::Steps::Solve:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "solve" );

        watch.start();
        repositories::solve();
        watch.stop();

        solveMeasurement.setValue( watch.getCalendarTime() );
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

