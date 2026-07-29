// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox::loadbalancing {
  class LoadBalancer;
  class Statistics;
} // namespace toolbox::loadbalancing
namespace toolbox::loadbalancing::tests {
  class LoadBalancerTest;
  class StatisticsTest;
} // namespace toolbox::loadbalancing::tests

/**
 * Collects the local grid statistics and turns them into a global view.
 *
 * The global view lags one grid sweep. A sweep reduces its own numbers after the
 * load-balancing decision, and the following sweep publishes that result before it decides,
 * so a decision always rests on the numbers of the sweep before it. Every rank reduces at the
 * same point of the sweep, and publishing is local, so a caller that only needs the current
 * view runs no communication at all.
 */
class toolbox::loadbalancing::Statistics final {
  friend class toolbox::loadbalancing::LoadBalancer;
  friend class toolbox::loadbalancing::tests::LoadBalancerTest;
  friend class toolbox::loadbalancing::tests::StatisticsTest;

public:
  static constexpr int NumberOfWords = 24;

  Statistics();
  Statistics(const Statistics&)            = delete;
  Statistics(Statistics&&)                 = delete;
  Statistics& operator=(const Statistics&) = delete;
  Statistics& operator=(Statistics&&)      = delete;
  ~Statistics();

  bool hasConsistentViewOfWorld() const;

  std::uint64_t getLocalNumberOfInnerUnrefinedCells() const;
  std::uint64_t getGlobalNumberOfInnerUnrefinedCells() const;

  std::uint64_t getLocalNumberOfInnerRefinedCells() const;
  std::uint64_t getGlobalNumberOfInnerRefinedCells() const;

  std::uint64_t getLocalNumberOfOuterUnrefinedCells() const;
  std::uint64_t getGlobalNumberOfOuterUnrefinedCells() const;

  std::uint64_t getLocalNumberOfVertices() const;
  std::uint64_t getGlobalNumberOfVertices() const;

  std::uint64_t getLocalNumberOfHangingVertices() const;
  std::uint64_t getGlobalNumberOfHangingVertices() const;

  int getLocalNumberOfTrees() const;
  int getGlobalNumberOfTrees() const;

  int getGlobalNumberOfSplits() const;

  int getGlobalNumberOfRanksWithAtLeastOneTree() const;

  int getGlobalMinTreeLoad() const;
  int getGlobalMaxTreeLoad() const;

  int getGlobalMinTreeOuterLoad() const;
  int getGlobalMaxTreeOuterLoad() const;

  std::uint64_t getGlobalMinRankLoad() const;
  std::uint64_t getGlobalMaxRankLoad() const;

  int getGlobalRankWithMaxTreeLoad() const;

  std::uint64_t getLocalMemoryUsageInBytes() const;
  std::uint64_t getGlobalMemoryUsageInBytes() const;
  std::uint64_t getLocalProcessMemoryUsageInBytes() const;
  std::uint64_t getGlobalProcessMemoryUsageInBytes() const;

  std::uint64_t getGlobalTotalMemoryInBytes() const;
  std::uint64_t getGlobalFreeMemoryInBytes() const;

  int getLocalMinDepth() const;
  int getGlobalMinDepth() const;

  int getLocalMaxDepth() const;
  int getGlobalMaxDepth() const;

  bool hasAdaptiveMesh() const;

  double getLocalMinH() const;
  double getGlobalMinH() const;

  int getLocalNumberOfStationarySweeps() const;
  int getGlobalNumberOfStationarySweeps() const;

  bool hasUniformGridCompleted() const;
  bool hasGlobalUniformGridCompleted() const;

private:
  void initialiseExchange();
  void shutdownMPI();

  void updateGlobalView();
  void synchroniseGlobalView();
  void startGlobalViewExchange();
  void publishGlobalView();
  void resetInitialDecompositionState();
  void incLocalNumberOfSplits(int delta = 1);
  void notifyUniformGridComplete();

  bool _isViewConsistent;
  bool _hasPublishedGlobalView;

  std::uint64_t _localNumberOfInnerUnrefinedCells;
  std::uint64_t _globalNumberOfInnerUnrefinedCells;

  std::uint64_t _localNumberOfInnerRefinedCells;
  std::uint64_t _globalNumberOfInnerRefinedCells;

  std::uint64_t _localNumberOfOuterUnrefinedCells;
  std::uint64_t _globalNumberOfOuterUnrefinedCells;

  std::uint64_t _localNumberOfVertices;
  std::uint64_t _globalNumberOfVertices;

  std::uint64_t _localNumberOfHangingVertices;
  std::uint64_t _globalNumberOfHangingVertices;

  int _localNumberOfTrees;
  int _globalNumberOfTrees;

  int _globalNumberOfRanksWithAtLeastOneTree;

  int _localNumberOfSplits;
  int _globalNumberOfSplits;

  int _localMinTreeLoad;
  int _localMaxTreeLoad;
  int _globalMinTreeLoad;
  int _globalMaxTreeLoad;
  int _globalRankWithMaxTreeLoad;

  int _localMinTreeOuterLoad;
  int _localMaxTreeOuterLoad;
  int _globalMinTreeOuterLoad;
  int _globalMaxTreeOuterLoad;

  std::uint64_t _globalMinRankLoad;
  std::uint64_t _globalMaxRankLoad;

  std::uint64_t _localMemoryUsageInBytes;
  std::uint64_t _globalMemoryUsageInBytes;
  std::uint64_t _localProcessMemoryUsageInBytes;
  std::uint64_t _globalProcessMemoryUsageInBytes;

  std::uint64_t _localTotalMemoryInBytes;
  std::uint64_t _localFreeMemoryInBytes;
  std::uint64_t _globalTotalMemoryInBytes;
  std::uint64_t _globalFreeMemoryInBytes;

  int _localMinDepth;
  int _localMaxDepth;
  int _globalMinDepth;
  int _globalMaxDepth;

  double _localMinH;
  double _globalMinH;

  int _localNumberOfStationarySweeps;
  int _globalNumberOfStationarySweeps;

  bool _localUniformGridComplete;
  bool _globalUniformGridComplete;

  /**
   * The reduced record of the preceding sweep, waiting to be published.
   */
  std::array<std::uint64_t, NumberOfWords> _reductionRecord{};

  bool _hasPendingRecord{false};
};
