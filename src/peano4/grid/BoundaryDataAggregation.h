// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"

namespace peano4::grid {
  template <class StackType>
  class BoundaryDataAggregation;
} // namespace peano4::grid

/**
 * Buffers behind the aggregated boundary exchange of one data container.
 *
 * A rank pair swaps one payload buffer per container and sweep, and both ranks
 * derive the same entity order and payload length, so a receiver reads the
 * message length off its own outgoing data. Each rank pair's record carries its
 * own sweep generation, so one rank's buffers may still travel while the next
 * sweep already packs towards another. A caller serialises all access to one
 * rank's record.
 */
template <class StackType>
class peano4::grid::BoundaryDataAggregation final {
public:
  using Stack = StackType;

  /**
   * Everything one rank pair exchanges in one sweep. transferComplete is set
   * by the poller that completes the posted messages and read by the
   * consumers of the received data.
   */
  struct RankExchange final {
    Stack             outgoingPayload{};
    Stack             incomingPayload{};
    std::atomic<bool> transferComplete{false};
    int               preparedGeneration{-1};
  };

  BoundaryDataAggregation()                                          = default;
  BoundaryDataAggregation(const BoundaryDataAggregation&)            = delete;
  BoundaryDataAggregation(BoundaryDataAggregation&&)                 = delete;
  BoundaryDataAggregation& operator=(const BoundaryDataAggregation&) = delete;
  BoundaryDataAggregation& operator=(BoundaryDataAggregation&&)      = delete;
  ~BoundaryDataAggregation()                                         = default;

  /**
   * Brings the rank records in line with the given sorted rank set. A version
   * the records already describe returns immediately, and an unchanged rank
   * set keeps the records, so a buffer keeps the capacity it grew to in
   * earlier sweeps. The single pack pass of a sweep is the only caller, so no
   * reader can overlap the rebuild.
   */
  void rebuildRankTable(const int topologyVersion, const std::vector<int>& remoteRanks) {
    assertion(std::is_sorted(remoteRanks.begin(), remoteRanks.end()));

    if (_topologyVersion == topologyVersion) {
      return;
    }

    if (_remoteRanks != remoteRanks) {
      _rankExchanges.clear();
      for (std::size_t entry = 0; entry < remoteRanks.size(); entry++) {
        _rankExchanges.emplace_back();
      }
      _remoteRanks = remoteRanks;
    }
    _topologyVersion = topologyVersion;
  }

  /**
   * Prepares one rank pair's record for the given sweep, unless it already
   * describes it.
   */
  void prepareRankExchange(const int generation, const int remoteRank) {
    auto& exchange = getRankExchange(remoteRank);
    if (exchange.preparedGeneration == generation) {
      return;
    }
    exchange.preparedGeneration = generation;
    exchange.outgoingPayload.shallowClear();
    exchange.incomingPayload.shallowClear();
    exchange.transferComplete.store(false, std::memory_order_relaxed);
  }

  RankExchange& getRankExchange(const int remoteRank) {
    const auto entry = std::lower_bound(_remoteRanks.begin(), _remoteRanks.end(), remoteRank);
    assertion(entry != _remoteRanks.end() and *entry == remoteRank, remoteRank);
    return _rankExchanges[static_cast<std::size_t>(entry - _remoteRanks.begin())];
  }

  /**
   * Releases the records. The payload buffers own the element storage that was
   * moved into them and the channels own persistent requests, so a container
   * that clears itself at shutdown has to take them down while MPI is still up
   * and before a leak check runs.
   */
  void clear() {
    _rankExchanges.clear();
    _remoteRanks.clear();
    _topologyVersion = -1;
  }

private:
  /**
   * A record holds the buffer an in-flight message reads from, so the records
   * must not move while the exchange runs, and a deque never relocates them.
   */
  std::deque<RankExchange> _rankExchanges{};

  /** Sorted, and running in lockstep with _rankExchanges. */
  std::vector<int> _remoteRanks{};

  /** Topology version the records describe. */
  int _topologyVersion{-1};
};
