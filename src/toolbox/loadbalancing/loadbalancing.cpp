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

  int sumLocal  = 0;
  int sumRemote = 0;
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  msg << idsOfLocalSpacetrees.size() << " tree(s): ";

  for (auto p: idsOfLocalSpacetrees) {
    msg << "(#" << p << ":"
        << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()
		<< "/"
		<< peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells()
		<< ")";
    sumLocal  += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
    sumRemote += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells();
  }
  if (idsOfLocalSpacetrees.empty()) {
    msg << "no trees hosted on this node - therefore no load";
  }
  else {
    msg << " total=" << sumLocal << "/" << sumRemote << " (local/remote)";
  }


  //msg << " used memory=" << tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte ) << " MByte, "
  //    << " free memory=" << tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte )  << " MByte";

  logInfo( "dumpStatistics()", msg.str() );
}


int toolbox::loadbalancing::getIdOfHeaviestLocalSpacetree() {
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  int result = NoHeaviestTreeAvailable;
  int maxLocalUnrefinedCells = -1;

  for (auto p: idsOfLocalSpacetrees) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>maxLocalUnrefinedCells
      and
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>=ThreePowerD
    ) {
      maxLocalUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
      result = p;
    }
  }
  return result;
}


int toolbox::loadbalancing::getIdOfHeaviestLocalSpacetree(double tolerance) {
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  int result = NoHeaviestTreeAvailable;
  int maxLocalUnrefinedCells = -1;

  for (auto p: idsOfLocalSpacetrees) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>maxLocalUnrefinedCells
      and
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>=ThreePowerD
    ) {
      maxLocalUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
    }
  }

  if (maxLocalUnrefinedCells>0) {
    logInfo( "getIdOfHeaviestLocalSpacetree(double)", "try to find one maximum tree which has weight close to " << maxLocalUnrefinedCells << ", i.e. has more than " << (tolerance * static_cast<double>(maxLocalUnrefinedCells)) << " cells"  );
    for (auto p: idsOfLocalSpacetrees) {
      if (
        static_cast<double>( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() ) >= tolerance * static_cast<double>(maxLocalUnrefinedCells)
        and
        peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>=ThreePowerD
        and
        (rand() % 2 == 1 or result == NoHeaviestTreeAvailable)
      ) {
        logInfo( "getIdOfHeaviestLocalSpacetree(double)", "tree " << p << " meets criterion with weight of " << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() );
        result = p;
      }
    }
  }
  return result;
}


int toolbox::loadbalancing::getWeightOfHeaviestLocalSpacetree() {
  const int heaviestSpacetree = getIdOfHeaviestLocalSpacetree();
  return heaviestSpacetree==NoHeaviestTreeAvailable ? -1 : peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(heaviestSpacetree).getNumberOfLocalUnrefinedCells();
}


