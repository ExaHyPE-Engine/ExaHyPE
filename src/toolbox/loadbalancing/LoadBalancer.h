// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/Blacklist.h"
#include "toolbox/loadbalancing/Configuration.h"
#include "toolbox/loadbalancing/State.h"
#include "toolbox/loadbalancing/Statistics.h"

namespace toolbox::loadbalancing {
  class LoadBalancer;
  struct InitialSplitPlan;
} // namespace toolbox::loadbalancing
namespace toolbox::loadbalancing::tests {
  class LoadBalancerTest;
} // namespace toolbox::loadbalancing::tests

class toolbox::loadbalancing::LoadBalancer final {
  friend class toolbox::loadbalancing::tests::LoadBalancerTest;

public:
  explicit LoadBalancer(
    std::unique_ptr<toolbox::loadbalancing::Configuration> configuration =
      std::make_unique<toolbox::loadbalancing::DefaultConfiguration>()
  );
  LoadBalancer(const LoadBalancer&)            = delete;
  LoadBalancer(LoadBalancer&&)                 = delete;
  LoadBalancer& operator=(const LoadBalancer&) = delete;
  LoadBalancer& operator=(LoadBalancer&&)      = delete;
  ~LoadBalancer();

  void shutdown();

  void enable(bool value);
  void update();
  void synchroniseGlobalView();

  bool isStagnating() const;
  bool needsGridToStayStationary() const;

  void notifyUniformGridComplete();
  bool hasUniformGridCompleted() const;

  int getMaxPermittedTreesPerRank() const;
  int getConfiguredNumberOfTrees() const;

  const toolbox::loadbalancing::Statistics& getStatistics() const;

  static std::string describeLocalDecomposition();

private:
  const std::unique_ptr<toolbox::loadbalancing::Configuration> _configuration;
  const int                                                    _initialTreesPerRank;
  toolbox::loadbalancing::Blacklist                            _blacklist;
  toolbox::loadbalancing::Statistics                           _statistics;
  toolbox::loadbalancing::State                                _state;

  class Strategy {
  public:
    Strategy()                           = default;
    Strategy(const Strategy&)            = delete;
    Strategy(Strategy&&)                 = delete;
    Strategy& operator=(const Strategy&) = delete;
    Strategy& operator=(Strategy&&)      = delete;
    virtual ~Strategy()                  = default;

    virtual toolbox::loadbalancing::State update(toolbox::loadbalancing::LoadBalancer& loadBalancer) = 0;
  };

  class InitialInterRankSplit;
  class InitialIntraRankSplit;

  std::unique_ptr<Strategy> _strategy;

  static std::unique_ptr<Strategy> makeInitialInterRankSplit();
  static std::unique_ptr<Strategy> makeInitialIntraRankSplit();

  void setState(toolbox::loadbalancing::State state);
  void updateBlacklist();
  void updateStrategy();

  int getHeaviestNonBlacklistedLocalSpacetreeId() const;

  bool mayTriggerSplits() const;

  int minPieceSizeForInitialSplits() const;

  void triggerSplits(int sourceTreeId, const toolbox::loadbalancing::InitialSplitPlan& plan);
};
