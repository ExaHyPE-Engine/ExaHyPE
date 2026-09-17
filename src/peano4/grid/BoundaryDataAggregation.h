// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/StackExchange.h"
#include "tarch/Assertions.h"

namespace peano4::grid {
  template <class StackType>
  class BoundaryDataAggregation;
} // namespace peano4::grid

/**
 * Stores aggregated boundary buffers for one data container.
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
  using Stack       = StackType;
  using ElementType = typename StackType::ElementType;

  /**
   * Stores both directions and protocol state for one rank pair and sweep.
   */
  struct RankExchange final {
    Stack                                       outgoingPayload{};
    Stack                                       incomingPayload{};
    peano4::grid::StackExchangeJob<ElementType> outgoingJob{};
    peano4::grid::StackExchangeJob<ElementType> incomingJob{};
    std::atomic<bool>                           transferComplete{false};
    int                                         preparedGeneration{-1};
  };

  BoundaryDataAggregation()                                          = default;
  BoundaryDataAggregation(const BoundaryDataAggregation&)            = delete;
  BoundaryDataAggregation(BoundaryDataAggregation&&)                 = delete;
  BoundaryDataAggregation& operator=(const BoundaryDataAggregation&) = delete;
  BoundaryDataAggregation& operator=(BoundaryDataAggregation&&)      = delete;
  ~BoundaryDataAggregation()                                         = default;

  /**
   * Rebuilds rank records for a changed topology version.
   *
   * An unchanged rank set preserves buffer capacity. Callers exclude
   * overlapping access.
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
   * Releases payload storage and request state before communication shuts down.
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

  //! Stores sorted ranks aligned with _rankExchanges.
  std::vector<int> _remoteRanks{};

  //! Identifies the topology described by the rank records.
  int _topologyVersion{-1};
};
