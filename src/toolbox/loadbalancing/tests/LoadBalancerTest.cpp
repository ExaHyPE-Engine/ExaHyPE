// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancerTest.h"

#include "peano4/Globals.h"
#include "tarch/mpi/Rank.h"
#include "toolbox/loadbalancing/Configuration.h"
#include "toolbox/loadbalancing/LoadBalancer.h"
#include "toolbox/loadbalancing/State.h"

namespace {
  class DeterministicConfiguration final : public toolbox::loadbalancing::Configuration {
  public:
    toolbox::loadbalancing::State getLastQueriedState() const { return _lastQueriedState; }

    int getMaxLocalTreesPerRank(const toolbox::loadbalancing::State state) const override {
      _lastQueriedState = state;

      switch (state) {
      case toolbox::loadbalancing::State::InitialInterRankSplit:
        return 7;
      case toolbox::loadbalancing::State::InterRankBalancing:
        return 11;
      case toolbox::loadbalancing::State::InitialIntraRankSplit:
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

    double getWorstCaseBalancingRatio(const toolbox::loadbalancing::State) const override { return 0.9; }

    peano4::grid::SplitInstruction::Mode getMode(const toolbox::loadbalancing::State) const override {
      return peano4::grid::SplitInstruction::Mode::BottomUp;
    }

  private:
    mutable toolbox::loadbalancing::State _lastQueriedState = toolbox::loadbalancing::State::SwitchedOff;
  };
} // namespace

toolbox::loadbalancing::tests::LoadBalancerTest::LoadBalancerTest() :
  tarch::tests::TestCase("toolbox::loadbalancing::tests::LoadBalancerTest") {}

void toolbox::loadbalancing::tests::LoadBalancerTest::run() {
  testMethod(testEnableDisableAndConfigurationHook);
  testMethod(testStationaryLevelWaitsOnlyWhenSplitCanStart);
  testMethod(testDescribeLocalDecomposition);
  testMethod(testInitialTargetIsStateIndependent);
  testMethod(testSettledInitialSplitStops);
  testMethod(testNullConfigurationIsRejected);
  testMethod(testStateOutput);
  testMethod(testUniformGridCompleteFlag);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testEnableDisableAndConfigurationHook() {
  auto                                 configuration     = std::make_unique<DeterministicConfiguration>();
  const auto* const                    configurationView = configuration.get();
  toolbox::loadbalancing::LoadBalancer loadBalancer(std::move(configuration));

  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::InitialInterRankSplit);
  validate(not loadBalancer.isStagnating());

  validateEquals(loadBalancer.getMaxPermittedTreesPerRank(), 7);
  validateEquals(configurationView->getLastQueriedState(), toolbox::loadbalancing::State::InitialInterRankSplit);

  loadBalancer.notifyUniformGridComplete();
  loadBalancer._statistics._isViewConsistent          = true;
  loadBalancer._statistics._hasPublishedGlobalView    = true;
  loadBalancer._statistics._globalNumberOfTrees       = 19;
  loadBalancer._statistics._localNumberOfSplits       = 2;
  loadBalancer._statistics._globalNumberOfSplits      = 4;
  loadBalancer._statistics._globalUniformGridComplete = true;
  loadBalancer._blacklist.notifySplit(23, 17);
  validate(loadBalancer.hasUniformGridCompleted());
  validate(loadBalancer._statistics.hasGlobalUniformGridCompleted());
  validate(loadBalancer._blacklist.isBlacklisted(23));

  loadBalancer.enable(false);
  validate(loadBalancer.isStagnating());
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::SwitchedOff);

  validateEquals(loadBalancer.getMaxPermittedTreesPerRank(), 7);
  validateEquals(configurationView->getLastQueriedState(), toolbox::loadbalancing::State::InitialInterRankSplit);

  loadBalancer.enable(true);
  validate(not loadBalancer.isStagnating());
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::InitialInterRankSplit);
  validate(not loadBalancer.hasUniformGridCompleted());
  validate(not loadBalancer._statistics.hasGlobalUniformGridCompleted());
  validate(not loadBalancer._blacklist.isBlacklisted(23));
  validate(loadBalancer._statistics._isViewConsistent);
  validate(loadBalancer._statistics._hasPublishedGlobalView);
  validateEquals(loadBalancer._statistics._globalNumberOfTrees, 19);
  validateEquals(loadBalancer._statistics._localNumberOfSplits, 0);
  validateEquals(loadBalancer._statistics._globalNumberOfSplits, 0);

  validateEquals(loadBalancer.getMaxPermittedTreesPerRank(), 7);
  validateEquals(configurationView->getLastQueriedState(), toolbox::loadbalancing::State::InitialInterRankSplit);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testStationaryLevelWaitsOnlyWhenSplitCanStart() {
  toolbox::loadbalancing::LoadBalancer loadBalancer(std::make_unique<DeterministicConfiguration>());
  auto&                                statistics   = loadBalancer._statistics;
  statistics._globalNumberOfSplits                  = 0;
  statistics._globalNumberOfStationarySweeps        = 0;
  statistics._globalNumberOfTrees                   = 1;
  statistics._globalNumberOfRanksWithAtLeastOneTree = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  statistics._globalMinDepth                        = 0;
  statistics._globalMaxDepth                        = 0;
  statistics._globalNumberOfInnerUnrefinedCells =
    loadBalancer.getConfiguredNumberOfTrees() * loadBalancer.minPieceSizeForInitialSplits() - 1;
  validate(not loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfInnerUnrefinedCells++;
  validate(loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfInnerUnrefinedCells = 0;
  statistics._globalNumberOfSplits              = 1;
  validate(loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfSplits           = 0;
  statistics._globalNumberOfStationarySweeps = -1;
  validate(loadBalancer.needsGridToStayStationary());

  loadBalancer.setState(toolbox::loadbalancing::State::Stagnation);
  validate(loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfStationarySweeps = 0;
  validate(not loadBalancer.needsGridToStayStationary());
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testDescribeLocalDecomposition() {
  const auto description = toolbox::loadbalancing::LoadBalancer::describeLocalDecomposition();
  validate(not description.empty());
  validate(description.find("Trees on this rank:") != std::string::npos);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testInitialTargetIsStateIndependent() {
  toolbox::loadbalancing::LoadBalancer loadBalancer(std::make_unique<DeterministicConfiguration>());
  validateEquals(loadBalancer.minPieceSizeForInitialSplits(), static_cast<int>(THREE_POWER_D));
  validateEquals(loadBalancer.getConfiguredNumberOfTrees(), 7 * tarch::mpi::Rank::getInstance().getNumberOfRanks());

  loadBalancer.setState(toolbox::loadbalancing::State::Stagnation);
  validateEquals(loadBalancer.getMaxPermittedTreesPerRank(), 7);
  validateEquals(loadBalancer.getConfiguredNumberOfTrees(), 7 * tarch::mpi::Rank::getInstance().getNumberOfRanks());
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testSettledInitialSplitStops() {
  toolbox::loadbalancing::LoadBalancer loadBalancer(std::make_unique<DeterministicConfiguration>());
  auto&                                statistics              = loadBalancer._statistics;
  const auto                           configuredNumberOfTrees = loadBalancer.getConfiguredNumberOfTrees();
  const auto                           numberOfRanks           = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  statistics._localNumberOfTrees                    = 0;
  statistics._globalNumberOfTrees                   = configuredNumberOfTrees;
  statistics._globalNumberOfInnerUnrefinedCells     = configuredNumberOfTrees * 2;
  statistics._globalNumberOfRanksWithAtLeastOneTree = numberOfRanks;
  statistics._globalMinDepth                        = 0;
  statistics._globalMaxDepth                        = 0;
  statistics._globalUniformGridComplete             = true;
  statistics._isViewConsistent                      = true;
  statistics._globalNumberOfSplits                  = 0;
  statistics._globalNumberOfStationarySweeps        = -1;
  loadBalancer.setState(toolbox::loadbalancing::State::InitialIntraRankSplit);
  loadBalancer.updateStrategy();
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::InitialIntraRankSplit);
  validate(loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfStationarySweeps = 0;
  statistics._isViewConsistent               = false;
  loadBalancer.updateStrategy();
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::InitialIntraRankSplit);
  validate(not loadBalancer.needsGridToStayStationary());

  statistics._isViewConsistent = true;
  loadBalancer.updateStrategy();
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::Stagnation);
  validate(not loadBalancer.needsGridToStayStationary());

  statistics._globalNumberOfTrees = configuredNumberOfTrees - 1;
  statistics._globalMinDepth      = 0;
  statistics._globalMaxDepth      = 1;
  loadBalancer.setState(toolbox::loadbalancing::State::InitialIntraRankSplit);
  loadBalancer.updateStrategy();
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::Stagnation);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testNullConfigurationIsRejected() {
  bool exceptionThrown = false;
  try {
    const toolbox::loadbalancing::LoadBalancer loadBalancer(std::unique_ptr<toolbox::loadbalancing::Configuration>{});
  } catch (const std::invalid_argument&) {
    exceptionThrown = true;
  }
  validate(exceptionThrown);
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testStateOutput() {
  std::ostringstream output;
  output << toolbox::loadbalancing::State::InitialInterRankSplit << " "
         << toolbox::loadbalancing::State::InitialIntraRankSplit << " "
         << toolbox::loadbalancing::State::InterRankBalancing << " "
         << toolbox::loadbalancing::State::IntraRankBalancing << " " << toolbox::loadbalancing::State::Stagnation << " "
         << toolbox::loadbalancing::State::SwitchedOff << " " << static_cast<toolbox::loadbalancing::State>(-1);
  validateEquals(
    output.str(),
    "InitialInterRankSplit InitialIntraRankSplit InterRankBalancing IntraRankBalancing Stagnation SwitchedOff Undefined"
  );
}

void toolbox::loadbalancing::tests::LoadBalancerTest::testUniformGridCompleteFlag() {
  toolbox::loadbalancing::LoadBalancer loadBalancer(std::make_unique<DeterministicConfiguration>());
  validate(not loadBalancer.hasUniformGridCompleted());
  validate(not loadBalancer._statistics.hasGlobalUniformGridCompleted());

  loadBalancer.notifyUniformGridComplete();
  validate(loadBalancer.hasUniformGridCompleted());
  validate(not loadBalancer._statistics.hasGlobalUniformGridCompleted());

  loadBalancer._statistics._localNumberOfTrees                    = 0;
  loadBalancer._statistics._globalNumberOfTrees                   = 1;
  loadBalancer._statistics._globalNumberOfRanksWithAtLeastOneTree = 1;
  loadBalancer._statistics._globalNumberOfInnerUnrefinedCells     = loadBalancer.getConfiguredNumberOfTrees();
  loadBalancer.updateStrategy();
  validateEquals(loadBalancer._state, toolbox::loadbalancing::State::InitialIntraRankSplit);

  loadBalancer._statistics.updateGlobalView();
  validate(loadBalancer.hasUniformGridCompleted());
  validate(loadBalancer._statistics.hasGlobalUniformGridCompleted());
}
