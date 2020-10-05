#include "TwoLevelGreedy.h"
#include "loadbalancing.h"

#include "peano4/utils/Globals.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tarch.h"
#include "tarch/multicore/Core.h"

#include "peano4/grid/GridStatistics.h"




tarch::logging::Log  toolbox::loadbalancing::TwoLevelGreedy::_log( "toolbox::loadbalancing::TwoLevelGreedy" );


toolbox::loadbalancing::TwoLevelGreedy::TwoLevelGreedy():
  _hasSpreadOutOverAllRanks( false ),
  _hasSpreadOutOverAllCores( false ),
  _hasSplitRecently(0) {
}


std::string toolbox::loadbalancing::TwoLevelGreedy::toString() const {
  std::ostringstream msg;

  msg << "(has-spread-over-all-ranks=" << _hasSpreadOutOverAllRanks
      << ",has-spread-over-all-cores=" << _hasSpreadOutOverAllCores
      << ")";

  return msg.str();
}


toolbox::loadbalancing::TwoLevelGreedy::~TwoLevelGreedy() {
}


void toolbox::loadbalancing::TwoLevelGreedy::finishSimulation() {
}


std::string toolbox::loadbalancing::TwoLevelGreedy::toString( StrategyStep step ) {
  switch ( step ) {
    case StrategyStep::Wait:
      return "wait";
    case StrategyStep::SpreadEquallyOverAllRanks:
      return "spread-equally-over-all-ranks";
    case StrategyStep::SpreadEquallyOverAllCores:
      return "spread-equally-over-all-cores";
    case StrategyStep::Completed:
      return "completed";
  }
  return "<undef>";
}


toolbox::loadbalancing::TwoLevelGreedy::StrategyStep toolbox::loadbalancing::TwoLevelGreedy::getStrategyStep() const {
  if ( _hasSpreadOutOverAllRanks and _hasSpreadOutOverAllCores ) {
    return StrategyStep::Completed;
  }


  const int localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();

  if (
    not _hasSpreadOutOverAllRanks
    and
    tarch::mpi::Rank::getInstance().getNumberOfRanks()>1
    and
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    localNumberOfInnerUnrefinedCells > tarch::mpi::Rank::getInstance().getNumberOfRanks()
  ) {
    return StrategyStep::SpreadEquallyOverAllRanks;
  }

  if (
    (_hasSpreadOutOverAllRanks or tarch::mpi::Rank::getInstance().getNumberOfRanks()==1)
    and
    not _hasSpreadOutOverAllCores
    and
    not peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().empty()
    and
    localNumberOfInnerUnrefinedCells > tarch::multicore::Core::getInstance().getNumberOfThreads()
  ) {
    return StrategyStep::SpreadEquallyOverAllCores;
  }

  return StrategyStep::Wait;
}


void toolbox::loadbalancing::TwoLevelGreedy::finishStep() {
  switch ( getStrategyStep() ) {
    case StrategyStep::Wait:
      break;
    case StrategyStep::SpreadEquallyOverAllRanks:
      {
        int localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();
        int cellsPerRank                     = std::max( localNumberOfInnerUnrefinedCells / tarch::mpi::Rank::getInstance().getNumberOfRanks(), 1);

        _hasSpreadOutOverAllRanks = true;

        for (int targetRank=1; targetRank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); targetRank++ ) {
          peano4::parallel::SpacetreeSet::getInstance().split(0,cellsPerRank,targetRank);
        }

        _hasSplitRecently = 4;
      }
      break;
    case StrategyStep::SpreadEquallyOverAllCores:
      {
        int localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();
        int cellsPerCore                     = std::max( localNumberOfInnerUnrefinedCells / tarch::multicore::Core::getInstance().getNumberOfThreads(), 1);
        int localTree                        = *peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin();

        _hasSpreadOutOverAllCores = true;
        _hasSpreadOutOverAllRanks = true;

        for (int targetCore=1; targetCore<tarch::multicore::Core::getInstance().getNumberOfThreads(); targetCore++ ) {
          peano4::parallel::SpacetreeSet::getInstance().split(localTree,cellsPerCore,tarch::mpi::Rank::getInstance().getRank());
        }

        _hasSplitRecently = 4;
      }
      break;
      break;
  }

  _hasSplitRecently--;

  dumpStatistics();
}




bool toolbox::loadbalancing::TwoLevelGreedy::hasSplitRecently() const {
  return _hasSplitRecently>0;
}


void toolbox::loadbalancing::TwoLevelGreedy::enable( bool value ) {
}
