#include "Noh-main.h"
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
#include "toolbox/particles/UnitTests.h"

#include "swift2/UserInterface.h"

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
#include "observers/Sort.h"


#include "observers/StephydroPartFlagBoundaryParticles_sweep0.h"

#include "observers/StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1.h"

#include "observers/StephydroPartForceLoopInitial_sweep2.h"

#include "observers/StephydroPartKick1_sweep3.h"

#include "observers/StephydroPartKickExtra1_sweep4.h"

#include "observers/StephydroPartDrift_sweep5.h"

#include "observers/StephydroPartPredictHydro_sweep6.h"

#include "observers/StephydroPartDensityLoopWithConstantSmoothingLength_sweep7.h"

#include "observers/StephydroPartForceLoop_sweep8.h"

#include "observers/StephydroPartKick2_sweep9.h"

#include "observers/StephydroPartKickExtra2_sweep10.h"

#include "observers/StephydroPartReduceGlobalQuantities_sweep11.h"



#include "tarch/timing/Measurement.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log _log("::");


tarch::timing::Measurement   createGridMeasurement;
tarch::timing::Measurement   initMeasurement;
tarch::timing::Measurement   sortMeasurement;
tarch::timing::Measurement   plotMeasurement;
tarch::timing::Measurement   timeStepMeasurement;


bool benchmarks::swift2::noh::selectNextAlgorithmicStep() {
  static bool   gridConstructed                             = false;
  static bool   gridInitialised                             = false;
  static bool   initialisationSorted                        = false;
  static double nextMaxPlotTimeStamp                        = benchmarks::swift2::noh::FirstPlotTimeStamp;
  static double nextMinPlotTimeStamp                        = benchmarks::swift2::noh::FirstPlotTimeStamp;
  static bool   haveJustWrittenSnapshot                     = false;
  static tarch::la::Vector<Dimensions,double> minH          = tarch::la::Vector<Dimensions,double>( std::numeric_limits<double>::max() );
  static int    globalNumberOfTrees                         = 0;
  bool          continueToSolve                             = true;

  if (false) {} // just there to make my script's syntax easier
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartFlagBoundaryParticles_sweep0
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartFlagBoundaryParticles_sweep0 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartFlagBoundaryParticles_sweep0
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartForceLoopInitial_sweep2 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartForceLoopInitial_sweep2
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartForceLoopInitial_sweep2 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartForceLoopInitial_sweep2
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKick1_sweep3 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKick1_sweep3
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKick1_sweep3 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKick1_sweep3
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKickExtra1_sweep4 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKickExtra1_sweep4
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKickExtra1_sweep4 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKickExtra1_sweep4
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDrift_sweep5 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDrift_sweep5
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDrift_sweep5 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDrift_sweep5
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartPredictHydro_sweep6 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartPredictHydro_sweep6
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartPredictHydro_sweep6 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartPredictHydro_sweep6
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartForceLoop_sweep8 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartForceLoop_sweep8
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartForceLoop_sweep8 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartForceLoop_sweep8
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKick2_sweep9 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKick2_sweep9
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKick2_sweep9 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKick2_sweep9
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKickExtra2_sweep10 )
    );
  }
  
  /**
   * Part of one complete time step cycle, i.e. one sequence of steps
   */
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKickExtra2_sweep10
    and
    repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartKickExtra2_sweep10 )
    );
  }
  else if (
    repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep())==repositories::StepRepository::Steps::StephydroPartKickExtra2_sweep10
    and
    not repositories::rerunPreviousGridSweep()
  ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartReduceGlobalQuantities_sweep11 )
    );
  }
  
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
        ". Stop grid construction"
      );
      gridConstructed = true;
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
  else if (not gridInitialised) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Init )
    );

    gridInitialised = true;
  }
  else if (not initialisationSorted) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Sort )
    );

    initialisationSorted = true;
  }
  else if (
    ( repositories::getMinTimeStamp()>=MinTerminalTime or repositories::getMaxTimeStamp()>=MaxTerminalTime )
    and
    ( TimeInBetweenPlots==0.0 or haveJustWrittenSnapshot )
  ) {
    continueToSolve         = false;
  }
  else if (
    TimeInBetweenPlots>0.0
    and
    (repositories::getMinTimeStamp()>=nextMinPlotTimeStamp or repositories::getMaxTimeStamp()>=nextMaxPlotTimeStamp)
    and
    not haveJustWrittenSnapshot
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
    haveJustWrittenSnapshot = true;
  }
  else if ( repositories::rerunPreviousGridSweep() ) {
    /*
    assertionEquals(
      repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()),
      repositories::StepRepository::Steps::StephydroPartReduceGlobalQuantities_sweep11
    );
    */

    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartReduceGlobalQuantities_sweep11 )
    );
  }
  else {
    /*
    assertionEquals(
      repositories::StepRepository::toStepEnum(peano4::parallel::Node::getInstance().getCurrentProgramStep()),
      repositories::StepRepository::Steps::StephydroPartReduceGlobalQuantities_sweep11
    );
    */

    peano4::parallel::Node::getInstance().setNextProgramStep(
      repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::StephydroPartFlagBoundaryParticles_sweep0 )
    );

    haveJustWrittenSnapshot = false;

    if (not continueToSolve and TimeInBetweenPlots>0.0) {
      continueToSolve       = true;
      nextMinPlotTimeStamp  = repositories::getMinTimeStamp();
    }
  }

  return continueToSolve;
}


tarch::timing::Watch  watch( "::", "step()", false );


void benchmarks::swift2::noh::step() {
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
    case repositories::StepRepository::Steps::Init:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "init" );
        watch.start();
        otterPhaseBegin( "init" );
        observers::Init  observer;
        observers::Init::prepareTraversal();
        repositories::startInitialisationStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishInitialisationStep();
        observers::Init::unprepareTraversal();
        watch.stop();
        initMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Sort:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "sort" );
        watch.start();
        otterPhaseBegin( "sort" );
        observers::Sort  observer;
        observers::Sort::prepareTraversal();
        repositories::startSortingStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishSortingStep();
        observers::Sort::unprepareTraversal();
        watch.stop();
        sortMeasurement.setValue( watch.getCalendarTime() );
      }
      break;
    case repositories::StepRepository::Steps::Plot:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "plot" );
        watch.start();
        otterPhaseSwitch( "plot" );
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
    case repositories::StepRepository::Steps::StephydroPartFlagBoundaryParticles_sweep0:
      {
        tarch::logging::LogFilter::getInstance().switchProgramPhase( "time-step" );
        watch.start();
        otterPhaseSwitch( "StephydroPartFlagBoundaryParticles_sweep0" );
        observers::StephydroPartFlagBoundaryParticles_sweep0  observer;
        observers::StephydroPartFlagBoundaryParticles_sweep0::prepareTraversal();
        repositories::startTimeStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartFlagBoundaryParticles_sweep0::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1:
      {
        otterPhaseSwitch( "StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1" );
        repositories::startIntermediateStep();
        observers::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1  observer;
        observers::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartForceLoopInitial_sweep2:
      {
        otterPhaseSwitch( "StephydroPartForceLoopInitial_sweep2" );
        repositories::startIntermediateStep();
        observers::StephydroPartForceLoopInitial_sweep2  observer;
        observers::StephydroPartForceLoopInitial_sweep2::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartForceLoopInitial_sweep2::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartKick1_sweep3:
      {
        otterPhaseSwitch( "StephydroPartKick1_sweep3" );
        repositories::startIntermediateStep();
        observers::StephydroPartKick1_sweep3  observer;
        observers::StephydroPartKick1_sweep3::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartKick1_sweep3::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartKickExtra1_sweep4:
      {
        otterPhaseSwitch( "StephydroPartKickExtra1_sweep4" );
        repositories::startIntermediateStep();
        observers::StephydroPartKickExtra1_sweep4  observer;
        observers::StephydroPartKickExtra1_sweep4::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartKickExtra1_sweep4::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartDrift_sweep5:
      {
        otterPhaseSwitch( "StephydroPartDrift_sweep5" );
        repositories::startIntermediateStep();
        observers::StephydroPartDrift_sweep5  observer;
        observers::StephydroPartDrift_sweep5::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartDrift_sweep5::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartPredictHydro_sweep6:
      {
        otterPhaseSwitch( "StephydroPartPredictHydro_sweep6" );
        repositories::startIntermediateStep();
        observers::StephydroPartPredictHydro_sweep6  observer;
        observers::StephydroPartPredictHydro_sweep6::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartPredictHydro_sweep6::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7:
      {
        otterPhaseSwitch( "StephydroPartDensityLoopWithConstantSmoothingLength_sweep7" );
        repositories::startIntermediateStep();
        observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7  observer;
        observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep7::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartForceLoop_sweep8:
      {
        otterPhaseSwitch( "StephydroPartForceLoop_sweep8" );
        repositories::startIntermediateStep();
        observers::StephydroPartForceLoop_sweep8  observer;
        observers::StephydroPartForceLoop_sweep8::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartForceLoop_sweep8::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartKick2_sweep9:
      {
        otterPhaseSwitch( "StephydroPartKick2_sweep9" );
        repositories::startIntermediateStep();
        observers::StephydroPartKick2_sweep9  observer;
        observers::StephydroPartKick2_sweep9::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartKick2_sweep9::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartKickExtra2_sweep10:
      {
        otterPhaseSwitch( "StephydroPartKickExtra2_sweep10" );
        repositories::startIntermediateStep();
        observers::StephydroPartKickExtra2_sweep10  observer;
        observers::StephydroPartKickExtra2_sweep10::prepareTraversal();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishIntermediateStep();
        observers::StephydroPartKickExtra2_sweep10::unprepareTraversal();
      }
      break;
    
    case repositories::StepRepository::Steps::StephydroPartReduceGlobalQuantities_sweep11:
      {
        otterPhaseSwitch( "StephydroPartReduceGlobalQuantities_sweep11" );
        observers::StephydroPartReduceGlobalQuantities_sweep11  observer;
        observers::StephydroPartReduceGlobalQuantities_sweep11::prepareTraversal();
        repositories::startIntermediateStep();
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        repositories::finishTimeStep();
        observers::StephydroPartReduceGlobalQuantities_sweep11::unprepareTraversal();
        watch.stop();
        timeStepMeasurement.setValue( watch.getCalendarTime() );
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

#ifdef UseFEExcept
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  peano4::initParallelEnvironment(&argc,&argv);
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

  peano4::initSingletons(
    benchmarks::swift2::noh::DomainOffset,
    benchmarks::swift2::noh::DomainSize,
    benchmarks::swift2::noh::PeriodicBC
  );

  benchmarks::swift2::noh::repositories::DataRepository::initDatatypes();

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

  otterTraceInitialise(OTTER_SRC_ARGS());


  #if defined(SharedOMP)
  #pragma omp parallel
  {
  #pragma omp master 
  {
  #endif
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    while ( benchmarks::swift2::noh::selectNextAlgorithmicStep() ) {
      benchmarks::swift2::noh::step();
    }

    logInfo("main()", "terminated successfully");
    logInfo("main()", "grid construction:  " << createGridMeasurement.getAccumulatedValue() << "s\t" << createGridMeasurement.toString() );
    logInfo("main()", "init simulation:    " << initMeasurement.getAccumulatedValue() << "s\t" << initMeasurement.toString() );
    logInfo("main()", "(initial) sort:     " << sortMeasurement.getAccumulatedValue() << "s\t" << initMeasurement.toString() );
    logInfo("main()", "plotting:           " << plotMeasurement.getAccumulatedValue() << "s\t" << plotMeasurement.toString() );
    logInfo("main()", "time stepping:      " << timeStepMeasurement.getAccumulatedValue() << "s\t" << timeStepMeasurement.toString() );
  }
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      benchmarks::swift2::noh::step();
    }
  }
  #if defined(SharedOMP)
  }
  }
  #endif

  peano4::shutdownSingletons();
  benchmarks::swift2::noh::repositories::DataRepository::shutdownDatatypes();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
