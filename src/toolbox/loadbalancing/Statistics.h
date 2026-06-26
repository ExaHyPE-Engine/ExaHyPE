// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/mpi/mpi.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace toolbox::loadbalancing {
  class Statistics;
} // namespace toolbox::loadbalancing

class toolbox::loadbalancing::Statistics {
public:
  static constexpr int NumberOfWords = 16;

  Statistics();
  ~Statistics();

  static void shutdownMPI();

  /**
   * Complete a still-in-flight non-blocking reduction (per instance), so no MPI request outlives
   * MPI_Finalize. Call this on every rank while MPI is still up, e.g. from LoadBalancer::shutdown().
   */
  void completePendingReduction();

  void updateGlobalView();
  bool hasConsistentViewOfWorld() const;

  int getLocalNumberOfInnerUnrefinedCells() const;
  int getGlobalNumberOfInnerUnrefinedCells() const;

  int getLocalNumberOfOuterUnrefinedCells() const;
  int getGlobalNumberOfOuterUnrefinedCells() const;

  int getLocalNumberOfVertices() const;
  int getGlobalNumberOfVertices() const;

  int getLocalNumberOfHangingVertices() const;
  int getGlobalNumberOfHangingVertices() const;

  int getLocalNumberOfTrees() const;
  int getGlobalNumberOfTrees() const;

  void incLocalNumberOfSplits(int delta = 1);
  int  getNumberOfStateUpdatesWithoutAnySplit() const;

  int getGlobalNumberOfRanksWithAtLeastOneTree() const;

  int getGlobalMinTreeLoad() const;
  int getGlobalMaxTreeLoad() const;

  int getGlobalRankWithMaxTreeLoad() const;

  std::size_t getLocalMemoryUsageInBytes() const;
  std::size_t getGlobalMemoryUsageInBytes() const;
  std::size_t getLocalProcessMemoryUsageInBytes() const;
  std::size_t getGlobalProcessMemoryUsageInBytes() const;

  int getLocalMinDepth() const;
  int getGlobalMinDepth() const;

  int getLocalMaxDepth() const;
  int getGlobalMaxDepth() const;

  double getLocalMinH() const;
  double getGlobalMinH() const;

  int getLocalNumberOfStationarySweeps() const;
  int getGlobalNumberOfStationarySweeps() const;

private:
  // Complete the in-flight non-blocking reduction launched last sweep and publish its (one-sweep
  // lagged) result. The request was launched a full sweep ago and overlaps the traversal, so this
  // does not block the hot path.
  void waitForGlobalDataExchange();

  bool _isViewConsistent;

  int _localNumberOfInnerUnrefinedCells;
  int _globalNumberOfInnerUnrefinedCells;

  int _localNumberOfOuterUnrefinedCells;
  int _globalNumberOfOuterUnrefinedCells;

  int _localNumberOfVertices;
  int _globalNumberOfVertices;

  int _localNumberOfHangingVertices;
  int _globalNumberOfHangingVertices;

  int _localNumberOfTrees;
  int _globalNumberOfTrees;

  int _globalNumberOfRanksWithAtLeastOneTree;

  int _localNumberOfSplits;
  int _globalNumberOfSplits;

  int _numberOfStateUpdatesWithoutAnySplit;

  int _localMinTreeLoad;
  int _localMaxTreeLoad;
  int _globalMinTreeLoad;
  int _globalMaxTreeLoad;
  int _globalRankWithMaxTreeLoad;

  std::uint64_t _localMemoryUsageInBytes;
  std::uint64_t _globalMemoryUsageInBytes;
  std::uint64_t _localProcessMemoryUsageInBytes;
  std::uint64_t _globalProcessMemoryUsageInBytes;

  int _localMinDepth;
  int _localMaxDepth;
  int _globalMinDepth;
  int _globalMaxDepth;

  double _localMinH;
  double _globalMinH;

  int _localNumberOfStationarySweeps;
  int _globalNumberOfStationarySweeps;

#ifdef PEANO_WITH_MPI
  // Pack everything into one record (uint64_t words), reduced via a custom MPI_Op.
  // Store the double as raw bits in a uint64_t.
  std::array<std::uint64_t, NumberOfWords> _globalExchangeSend{};
  std::array<std::uint64_t, NumberOfWords> _globalExchangeRecv{};
  MPI_Request                              _globalExchangeRequest;
  bool                                     _globalExchangeInFlight;
#endif
};
