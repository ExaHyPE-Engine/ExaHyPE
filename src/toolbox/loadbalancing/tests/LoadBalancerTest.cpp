// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancerTest.h"

#include "toolbox/loadbalancing/Configuration.h"
#include "toolbox/loadbalancing/LoadBalancer.h"
#include "toolbox/loadbalancing/State.h"

#include <string>

namespace {
  /**
   * Deterministic configuration used by the unit tests.
   *
   * The configuration interface is state-dependent, so we record the last state
   * that the load balancer queried to verify correct plumbing.
   */
  class DeterministicConfiguration final: public toolbox::loadbalancing::Configuration {
  public:
    mutable toolbox::loadbalancing::State lastQueriedState = toolbox::loadbalancing::State::SwitchedOff;

    int getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) const override {
      lastQueriedState = state;

      // Return different values per state so the test can verify that the load
      // balancer forwards its current state into the configuration.
      switch (state) {
      case toolbox::loadbalancing::State::InterRankDistribution:
        return 7;
      case toolbox::loadbalancing::State::InterRankBalancing:
        return 11;
      case toolbox::loadbalancing::State::IntraRankDistribution:
        return 13;
      case toolbox::loadbalancing::State::IntraRankBalancing:
        return 17;
      case toolbox::loadbalancing::State::Stagnation:
        return 19;
      case toolbox::loadbalancing::State::SwitchedOff:
        return 23;
      }

      return 7;
    }

    double getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const override { return 0.9; }

    peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State) const override {
      return peano4::grid::SplitInstruction::Mode::BottomUp;
    }

    int getMinTreeSize(toolbox::loadbalancing::State) const override { return 1; }
  };
} // namespace

toolbox::loadbalancing::tests::LoadBalancerTest::LoadBalancerTest():
  tarch::tests::TestCase("toolbox::loadbalancing::tests::LoadBalancerTest") {}

void toolbox::loadbalancing::tests::LoadBalancerTest::run() {
  testMethod(testEnableDisableAndConfigurationHook);
  testMethod(testHasSplitRecentlyThreshold);
  testMethod(testSettlingIterations);
  testMethod(testLocalLoadBalancingString);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testEnableDisableAndConfigurationHook() {
  auto*        config = new DeterministicConfiguration();
  LoadBalancer lb(config);

  // Constructor should initialise deterministically.
  validateEquals(lb._state, State::InterRankDistribution);
  validate(not lb.isStagnating());

  // Verify LoadBalancer forwards its current state into Configuration.
  validateEquals(lb.getMaxPermittedTreesPerRank(), 7);
  validateEquals(config->lastQueriedState, State::InterRankDistribution);

  lb.enable(false);
  validate(lb.isStagnating());
  validateEquals(lb._state, State::SwitchedOff);

  validateEquals(lb.getMaxPermittedTreesPerRank(), 23);
  validateEquals(config->lastQueriedState, State::SwitchedOff);

  lb.enable(true);
  validate(not lb.isStagnating());
  // Re-enabling from SwitchedOff transitions into InterRankBalancing per implementation.
  validateEquals(lb._state, State::InterRankBalancing);

  validateEquals(lb.getMaxPermittedTreesPerRank(), 11);
  validateEquals(config->lastQueriedState, State::InterRankBalancing);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testHasSplitRecentlyThreshold() {
  toolbox::loadbalancing::LoadBalancer lb(new DeterministicConfiguration());

  // Initially, the counter is 0 => hasSplitRecently() should be true.
  validateEquals(lb._statistics.getNumberOfStateUpdatesWithoutAnySplit(), 0);
  validate(lb.hasSplitRecently());

  // Drive the Statistics counter without invoking LoadBalancer::update() (which might split).
  lb._statistics.updateGlobalView(); // -> 1
  validateEquals(lb._statistics.getNumberOfStateUpdatesWithoutAnySplit(), 1);
  validate(lb.hasSplitRecently());

  lb._statistics.updateGlobalView(); // -> 2
  validateEquals(lb._statistics.getNumberOfStateUpdatesWithoutAnySplit(), 2);
  validate(lb.hasSplitRecently());

  lb._statistics.updateGlobalView(); // -> 3
  validateEquals(lb._statistics.getNumberOfStateUpdatesWithoutAnySplit(), 3);
  validate(not lb.hasSplitRecently());

  // A (local) split should reset the counter to 0.
  lb._statistics.incLocalNumberOfSplits(1);
  validateEquals(lb._statistics.getNumberOfStateUpdatesWithoutAnySplit(), 0);
  validate(lb.hasSplitRecently());
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testSettlingIterations() {
  toolbox::loadbalancing::LoadBalancer lb(new DeterministicConfiguration());
  validateEquals(lb.getNumberOfSettlingIterations(), 5);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testLocalLoadBalancingString() {
  const std::string s = toolbox::loadbalancing::LoadBalancer::getLocalLoadBalancing();
  validate(not s.empty());
  validate(s.find("tree(s):") != std::string::npos);
}
