// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Blacklist.h"
#include "Configuration.h"
#include "State.h"
#include "Statistics.h"

#include <set>
#include <string>

namespace toolbox::loadbalancing {
  class LoadBalancer;
} // namespace toolbox::loadbalancing

class toolbox::loadbalancing::LoadBalancer {
public:
  LoadBalancer(Configuration* configuration = new DefaultConfiguration);
  ~LoadBalancer();

  void enable(bool value);
  bool isEnabled(bool globally) const;

  void update();

  int         getMaxPermittedTreesPerRank() const;
  int         getGlobalNumberOfTrees() const;
  int         getGlobalNumberOfInnerUnrefinedCells() const;
  int         getGlobalNumberOfOuterUnrefinedCells() const;
  int         getGlobalNumberOfVertices() const;
  int         getGlobalNumberOfHangingVertices() const;
  int         getGlobalMinTreeLoad() const;
  int         getGlobalMaxTreeLoad() const;
  std::size_t getGlobalMemoryUsageInBytes() const;
  int         getGlobalMinDepth() const;
  int         getGlobalMaxDepth() const;
  double      getGlobalMinH() const;
  int         getGlobalNumberOfStationarySweeps() const;

  bool hasStagnated() const;
  bool hasSplitRecently() const;

  int getNumberOfSettlingIterations() const;

  std::string toString() const;

  static std::string getStatistics();

private:
  static tarch::logging::Log _log;

  Configuration* _configuration;
  Blacklist      _blacklist;
  Statistics     _statistics;
  State          _state;

  void updateInterRankLoadBalancing();
  void updateIntraRankLoadBalancing();

  /**
   * Finds the local spacetree with the highest number of local unrefined cells
   * that is currently not on the blacklist.
   * @return Tree ID, or -1 if no suitable tree is found.
   */
  int getHeaviestNonBlacklistedLocalSpacetreeID() const;

  void triggerSplit(int numberOfCells, int targetRank, int sourceTree);
};
