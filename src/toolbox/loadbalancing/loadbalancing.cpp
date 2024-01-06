#include "loadbalancing.h"

#include "tarch/logging/Log.h"

#include "peano4/parallel/SpacetreeSet.h"

#include "peano4/grid/GridStatistics.h"

#include "tarch/tarch.h"

#include <stdlib.h>


namespace {
  // If I don't instantiate it here, i.e. outside of a function, then I run risk
  // that it is initialised late and thus has the wrong time stamp.
  tarch::logging::Log _log( "toolbox::loadbalancing" );
}


void toolbox::loadbalancing::dumpStatistics() {
  std::ostringstream msg;

  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  int localMinOfUnrefinedCells = std::numeric_limits<int>::max();
  int localMaxOfUnrefinedCells = 0;
  int localSumOfUnrefinedCells  = 0;
  int localSumOfRemoteUnrefinedCells = 0;
  int localSumOfSpacetrees = idsOfLocalSpacetrees.size();

  msg << localSumOfSpacetrees << " Local Tree(s): ";

  for (const auto& p: idsOfLocalSpacetrees) {
    msg << "(#" << p << ":";
    msg << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
    msg << "/";
    msg << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells();
    msg << ")";

    localMinOfUnrefinedCells = std::min(localMinOfUnrefinedCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells());
    localMaxOfUnrefinedCells = std::max(localMaxOfUnrefinedCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells());
    localSumOfUnrefinedCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
    localSumOfRemoteUnrefinedCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells();
  }

  if (idsOfLocalSpacetrees.empty()) {
    msg << "No trees hosted on this node - therefore no load.";
  } else {
    if (localSumOfRemoteUnrefinedCells > 0) {
      double granularity = static_cast<double>(localSumOfUnrefinedCells) / static_cast<double>(localSumOfRemoteUnrefinedCells);
      msg << ", Granularity=" << granularity; // Higher is better
    }
    msg << ", Ratio=" << localMinOfUnrefinedCells << "/" << localMaxOfUnrefinedCells << " (Min/Max)";
    msg << ", Total=" << localSumOfUnrefinedCells << "/" << localSumOfRemoteUnrefinedCells << " (Local/Remote)";
  }

  logInfo("dumpStatistics()", msg.str());

  int globalMinOfUnrefinedCells = std::numeric_limits<int>::max();
  int globalMaxOfUnrefinedCells = 0;
  int globalSumOfUnrefinedCells  = 0;
  int globalSumOfRemoteUnrefinedCells = 0;
  int globalSumOfSpacetrees = 0;

#ifdef Parallel
  tarch::mpi::Rank::getInstance().reduce(
    &localSumOfSpacetrees,
    &globalSumOfSpacetrees,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &localSumOfUnrefinedCells,
    &globalSumOfUnrefinedCells,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &localSumOfRemoteUnrefinedCells,
    &globalSumOfRemoteUnrefinedCells,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &localMinOfUnrefinedCells,
    &globalMinOfUnrefinedCells,
    1,
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &localMaxOfUnrefinedCells,
    &globalMaxOfUnrefinedCells,
    1,
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
#else
  globalMinOfUnrefinedCells = localMinOfUnrefinedCells;
  globalMaxOfUnrefinedCells = localMaxOfUnrefinedCells;
  globalSumOfUnrefinedCells  = localSumOfUnrefinedCells;
  globalSumOfRemoteUnrefinedCells = localSumOfRemoteUnrefinedCells;
  globalSumOfSpacetrees = localSumOfSpacetrees;
#endif

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::stringstream msg;
    msg << globalSumOfSpacetrees << " Global Tree(s):";
    if (globalSumOfRemoteUnrefinedCells > 0) {
      double granularity = static_cast<double>(globalSumOfUnrefinedCells) / static_cast<double>(globalSumOfRemoteUnrefinedCells);
      msg << " Granularity=" << granularity;
      msg << ", ";
    }
    msg << " Ratio=" << globalMinOfUnrefinedCells << "/" << globalMaxOfUnrefinedCells << " (Min/Max)";
    msg << ", Total=" << globalSumOfUnrefinedCells << "/" << globalSumOfRemoteUnrefinedCells << " (Local/Remote)";
    logInfo("dumpStatistics()", msg.str());
  }
}


int toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree() {
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  int heaviestSpacetree      = -1;
  int maxLocalUnrefinedCells = -1;

  for (auto p: idsOfLocalSpacetrees) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>maxLocalUnrefinedCells
      and
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>=ThreePowerD
    ) {
      maxLocalUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
      heaviestSpacetree = p;
    }
  }

  logDebug( "getWeightOfHeaviestLocalSpacetree()", "id=" << heaviestSpacetree << ", #octants=" << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(heaviestSpacetree).getNumberOfLocalUnrefinedCells() );
  return heaviestSpacetree==-1 ? -1 : peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(heaviestSpacetree).getNumberOfLocalUnrefinedCells();
}
