// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Blacklist.h"
#include "Configuration.h"
#include "State.h"
#include "Statistics.h"

#include <memory>
#include <set>
#include <string>

namespace toolbox::loadbalancing {
  class LoadBalancer;
  namespace tests {
    class LoadBalancerTest;
  } // namespace tests
} // namespace toolbox::loadbalancing

class toolbox::loadbalancing::LoadBalancer {
  friend class tests::LoadBalancerTest;

public:
  LoadBalancer(Configuration* configuration = new DefaultConfiguration);
  ~LoadBalancer();

  void enable(bool value);
  void update();

  bool isStagnating() const;
  bool hasSplitRecently() const;

  int getMaxPermittedTreesPerRank() const;

  const Statistics& getStatistics() const;

  static std::string getLocalLoadBalancing();

private:
  Configuration* _configuration;
  Blacklist      _blacklist;
  Statistics     _statistics;
  State          _state;

  class Strategy {
  public:
    virtual ~Strategy()                    = default;
    virtual State update(LoadBalancer& lb) = 0;
  };

  class InterRankDistributionStrategy;
  class IntraRankDistributionStrategy;

  std::unique_ptr<Strategy> _strategy;

  static std::unique_ptr<Strategy> makeInterRankDistributionStrategy();
  static std::unique_ptr<Strategy> makeIntraRankDistributionStrategy();

  void setState(State state);

  /**
   * Finds the local spacetree with the highest number of local unrefined cells
   * that is currently not on the blacklist.
   * @return Tree ID, or -1 if no suitable tree is found.
   */
  int getHeaviestNonBlacklistedLocalSpacetreeID() const;

  void triggerSplit(int numberOfCells, int targetRank, int sourceTree);
};
