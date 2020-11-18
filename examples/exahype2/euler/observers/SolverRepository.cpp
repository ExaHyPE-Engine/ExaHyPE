#include "SolverRepository.h"

#include <algorithm>

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"



namespace examples {namespace exahype2 {namespace euler {namespace observers {

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

tarch::logging::Log _log( "SolverRepository" );

::exahype2::RefinementControl  refinementControl;



peano4::grid::GridStatistics   gridStatisticsAfterGridConstruction;


toolbox::loadbalancing::RecursiveSubdivision              loadBalancer;
Euler InstanceOfEuler;



double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfEuler.getMinTimeStamp() );
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  result = std::max( result, InstanceOfEuler.getMaxTimeStamp() );
  return result;
}


double getMinMeshSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfEuler.getMinMeshSize() );
  return result;
}


double getMaxMeshSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfEuler.getMaxMeshSize() );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfEuler.getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfEuler.getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  InstanceOfEuler.startGridConstructionStep();

  refinementControl.clear();
}


void startGridInitialisationStep() {
  InstanceOfEuler.startGridInitialisationStep();
}


void startTimeStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfEuler.startTimeStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);

  refinementControl.clear();
}


void startPlottingStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfEuler.startPlottingStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
}


void finishTimeStep() {
  InstanceOfEuler.finishTimeStep();

  refinementControl.finishStep();
  loadBalancer.finishStep();
}


void finishGridConstructionStep() {
  InstanceOfEuler.finishGridConstructionStep();

  refinementControl.finishStep();
  loadBalancer.finishStep();

  gridStatisticsAfterGridConstruction = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  #ifdef Parallel
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++ ) {
      if (rank!=tarch::mpi::Rank::getGlobalMasterRank()) {
        peano4::grid::GridStatistics rankLocalStats;
        logDebug( "finishGridConstructionStep()", "receive grid statistics from rank " << rank << " via tag " << statisticsExchangeTag );
        peano4::grid::GridStatistics::receive(
          rankLocalStats,
          rank,
          statisticsExchangeTag,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        gridStatisticsAfterGridConstruction = gridStatisticsAfterGridConstruction + rankLocalStats;
      }
    }
  }
  else {
    logDebug( "finishGridConstructionStep()", "send out local statistics " << gridStatisticsAfterGridConstruction.toString() << " to global master via tag " << statisticsExchangeTag );
    peano4::grid::GridStatistics::send(
      gridStatisticsAfterGridConstruction,
      tarch::mpi::Rank::getInstance().getGlobalMasterRank(),
      statisticsExchangeTag,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
  #endif
}


void finishGridInitialisationStep() {
  InstanceOfEuler.finishGridInitialisationStep();
}


void finishPlottingStep() {
  InstanceOfEuler.finishPlottingStep();
}


void finishSimulation() {
  loadBalancer.finishStep();
  refinementControl.finishStep();
}


}}}}
