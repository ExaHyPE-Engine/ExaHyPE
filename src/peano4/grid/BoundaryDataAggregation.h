// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/mpi/PersistentChannel.h"

#include <mpi.h>

namespace peano4::grid {
  template <class StackType>
  class BoundaryDataAggregation;
} // namespace peano4::grid

/**
 * Buffers behind the aggregated boundary exchange of one data container.
 *
 * The horizontal exchange runs between pairs of trees, and a rank holds many
 * trees, so a rank pair would otherwise swap one small message per tree pair.
 * This class collects all of those into one buffer per remote rank: every tree
 * copies its payload into the buffer and describes it by a segment, and the
 * whole buffer travels as a single message.
 *
 * A receiver derives both message lengths from its own outgoing data, because
 * the exchange is symmetric: the number of elements tree A sends to tree B
 * equals the number B sends to A, and hence the number of non-empty pairs
 * between two ranks is the same on both sides. The segment table carries only
 * the identity of each segment, which frees the two ranks from having to agree
 * on the order in which the trees packed their data.
 */
template <class StackType>
class peano4::grid::BoundaryDataAggregation final {
public:
  using Stack = StackType;

  /**
   * One tree pair's contribution to an aggregated message. The three integers
   * sit next to each other without padding, so a table travels as raw bytes.
   */
  struct Segment final {
    int senderTreeId{-1};
    int receiverTreeId{-1};
    int numberOfElements{0};
  };

  /**
   * Everything one rank pair exchanges in one sweep.
   *
   * A tree that has no data for the remote rank this sweep still counts as a
   * contributor and decrements the counter, so the tree that takes it to zero
   * is the one that has seen every contribution and can post the messages.
   */
  struct RankExchange final {
    Stack                         outgoingPayload{};
    Stack                         incomingPayload{};
    std::vector<Segment>          outgoingSegments{};
    std::vector<Segment>          incomingSegments{};
    tarch::mpi::PersistentChannel outgoingSegmentChannel{};
    tarch::mpi::PersistentChannel incomingSegmentChannel{};
    std::atomic<int>              outstandingContributors{0};

#if PEANO_DEBUG
    int         outgoingSegmentCount{0};
    int         incomingSegmentCount{0};
    MPI_Request outgoingSegmentCountRequest{MPI_REQUEST_NULL};
    MPI_Request incomingSegmentCountRequest{MPI_REQUEST_NULL};
#endif
  };

  BoundaryDataAggregation()                                          = default;
  ~BoundaryDataAggregation()                                         = default;
  BoundaryDataAggregation(const BoundaryDataAggregation&)            = delete;
  BoundaryDataAggregation& operator=(const BoundaryDataAggregation&) = delete;
  BoundaryDataAggregation(BoundaryDataAggregation&&)                 = delete;
  BoundaryDataAggregation& operator=(BoundaryDataAggregation&&)      = delete;

  /**
   * Prepare the buffers for one sweep, unless they already describe it.
   *
   * remoteRanks is sorted and holds every rank this rank exchanges boundary
   * data with; contributorsPerRemoteRank runs in lockstep with it and holds,
   * per entry, how many local trees have at least one neighbour on that rank.
   * The generation identifies the sweep the two describe, so the first tree
   * that reaches the exchange prepares the container and the rest find it ready.
   *
   * As long as the rank set is unchanged the records survive, so a buffer keeps
   * the capacity it grew to in earlier sweeps. A changed rank set discards them,
   * which only happens when the partition changes.
   */
  void prepare(int generation, const std::vector<int>& remoteRanks, const std::vector<int>& contributorsPerRemoteRank) {
    assertion(remoteRanks.size() == contributorsPerRemoteRank.size());
    assertion(std::is_sorted(remoteRanks.begin(), remoteRanks.end()));

    if (_preparedGeneration == generation) {
      return;
    }
    _preparedGeneration = generation;

    if (_remoteRanks != remoteRanks) {
      _rankExchanges.clear();
      for (std::size_t i = 0; i < remoteRanks.size(); i++) {
        _rankExchanges.emplace_back();
      }
      _remoteRanks = remoteRanks;
    }

    for (std::size_t i = 0; i < _rankExchanges.size(); i++) {
      auto& exchange = _rankExchanges[i];
      exchange.outgoingPayload.shallowClear();
      exchange.incomingPayload.shallowClear();
      exchange.outgoingSegments.clear();
      exchange.incomingSegments.clear();
      exchange.outstandingContributors.store(contributorsPerRemoteRank[i], std::memory_order_relaxed);
#if PEANO_DEBUG
      exchange.outgoingSegmentCount = 0;
      exchange.incomingSegmentCount = 0;
#endif
    }
  }

  /**
   * Copy one tree pair's outgoing data into the buffer of its remote rank and
   * describe it by a segment. The source stack is emptied, as its data now
   * lives in the buffer that will be sent.
   */
  void appendOutgoing(RankExchange& exchange, int senderTreeId, int receiverTreeId, Stack& source) {
    const std::size_t numberOfElements = source.size();
    assertion(numberOfElements > 0, senderTreeId, receiverTreeId);

    auto destination = exchange.outgoingPayload.pushBlock(numberOfElements);
    for (std::size_t i = 0; i < numberOfElements; i++) {
      destination.set(i, std::move(source[i]));
    }
    source.shallowClear();

    exchange.outgoingSegments.push_back(Segment{senderTreeId, receiverTreeId, static_cast<int>(numberOfElements)});
  }

  /**
   * Whether the buffers describe the given sweep. A phase that runs before the
   * boundary exchange of a sweep finds them still describing an earlier one,
   * and has nothing of its own to complete here.
   */
  bool isPreparedFor(int generation) const { return _preparedGeneration == generation; }

  RankExchange& getRankExchange(int remoteRank) {
    const auto entry = std::lower_bound(_remoteRanks.begin(), _remoteRanks.end(), remoteRank);
    assertion(entry != _remoteRanks.end() and *entry == remoteRank, remoteRank);
    return _rankExchanges[static_cast<std::size_t>(entry - _remoteRanks.begin())];
  }

  /**
   * Release the records. The payload buffers own the element storage that was
   * moved into them and the channels own persistent requests, so a container
   * that clears itself at shutdown has to take them down while MPI is still up
   * and before a leak check runs.
   */
  void clear() {
    _rankExchanges.clear();
    _remoteRanks.clear();
    _preparedGeneration = -1;
  }

private:
  /**
   * A record holds the buffer an in-flight message reads from, so the records
   * must not move while the exchange runs. A deque only ever appends, whereas a
   * vector relocates its elements when it grows.
   */
  std::deque<RankExchange> _rankExchanges;

  /** Sorted, and running in lockstep with _rankExchanges. */
  std::vector<int> _remoteRanks;

  /** Sweep the records describe; -1 before the first prepare(). */
  int _preparedGeneration{-1};
};
