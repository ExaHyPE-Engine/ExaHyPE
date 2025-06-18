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

  int getGlobalNumberOfTrees() const;
  int getGlobalNumberOfInnerUnrefinedCells() const;

  bool hasStagnated() const;
  bool hasSplitRecently() const;

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

  void triggerSplit(int numberOfCells, int targetRank);
};
