// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementRequests.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  /**
   * Erase vote of one child cell, with the level and lattice position of its parent.
   */
  struct EraseVote final {
    int                                level{0};
    tarch::la::Vector<DIMENSIONS, int> lattice{0};
    int                                child{0};
    int                                slot{-1};

    [[nodiscard]] bool hasTheSameParentAs(const EraseVote& other) const noexcept {
      if (level != other.level) {
        return false;
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (lattice(axis) != other.lattice(axis)) {
          return false;
        }
      }
      return true;
    }

    bool operator<(const EraseVote& other) const noexcept {
      if (level != other.level) {
        return level < other.level;
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (lattice(axis) != other.lattice(axis)) {
          return lattice(axis) < other.lattice(axis);
        }
      }
      return child < other.child;
    }
  };

  std::uint8_t combine(const std::uint8_t vote, const exahype2::RefinementCommand command) {
    const auto     commandVote = static_cast<std::uint8_t>(command);
    constexpr auto Refine      = static_cast<std::uint8_t>(exahype2::RefinementCommand::Refine);
    constexpr auto Keep        = static_cast<std::uint8_t>(exahype2::RefinementCommand::Keep);
    if (vote == Refine or commandVote == Refine) {
      return Refine;
    }
    if (vote == Keep or commandVote == Keep) {
      return Keep;
    }
    return commandVote;
  }

  /**
   * Returns the erase votes of the cells below the root, sorted by parent and then by child.
   */
  std::vector<EraseVote> getSortedEraseVotes(
    const std::vector<std::uint8_t>& votes,
    const peano4::CellConnectivity&  cellConnectivity
  ) {
    constexpr auto         Erase = static_cast<std::uint8_t>(exahype2::RefinementCommand::Erase);
    std::vector<EraseVote> eraseVotes;
    for (const int slot : cellConnectivity.getCells()) {
      if (votes[static_cast<std::size_t>(slot)] != Erase or cellConnectivity.getCellLevel(slot) == 0) {
        continue;
      }
      EraseVote  eraseVote;
      const auto lattice = cellConnectivity.getCellLattice(slot);
      eraseVote.level    = cellConnectivity.getCellLevel(slot) - 1;
      eraseVote.slot     = slot;
      tarch::la::Vector<DIMENSIONS, int> childPosition;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        eraseVote.lattice(axis) = lattice(axis) / peano4::SubdivisionFactor;
        childPosition(axis)     = lattice(axis) % peano4::SubdivisionFactor;
      }
      eraseVote.child = peano4::dLinearised(childPosition, peano4::SubdivisionFactor);
      eraseVotes.push_back(eraseVote);
    }
    std::sort(eraseVotes.begin(), eraseVotes.end());
    return eraseVotes;
  }

  /**
   * Calls visit with the index of the first vote of every parent whose children all vote Erase.
   * eraseVotes is sorted by parent and then by child.
   */
  template <typename Visitor>
  void forEachCompleteSiblingSet(const std::vector<EraseVote>& eraseVotes, const Visitor& visit) {
    for (std::size_t vote = 0; vote < eraseVotes.size();) {
      std::size_t next = vote + 1;
      while (next < eraseVotes.size() and eraseVotes[next].hasTheSameParentAs(eraseVotes[vote])) {
        assertion(
          eraseVotes[next].child > eraseVotes[next - 1].child,
          eraseVotes[next].child,
          eraseVotes[next - 1].child
        );
        next++;
      }
      if (next - vote == static_cast<std::size_t>(peano4::ChildrenPerCell)) {
        visit(vote);
      }
      vote = next;
    }
  }
} // namespace

void
exahype2::RefinementRequests::clear(const int numberOfSlots) {
  assertion(numberOfSlots >= 0, numberOfSlots);
  _votes.assign(static_cast<std::size_t>(numberOfSlots), NoVote);
}

void
exahype2::RefinementRequests::merge(
  const std::vector<exahype2::RefinementCommand>& commands,
  const tarch::Span<const int>                    cells
) {
  for (const int slot : cells) {
    assertion(slot >= 0 and static_cast<std::size_t>(slot) < commands.size(), slot, commands.size());
    const auto index = static_cast<std::size_t>(slot);
    assertion(index < _votes.size(), index, _votes.size());
    _votes[index] = combine(_votes[index], commands[index]);
  }
}

void
exahype2::RefinementRequests::merge(const exahype2::RefinementCommand command, const tarch::Span<const int> cells) {
  for (const int slot : cells) {
    assertion(slot >= 0 and static_cast<std::size_t>(slot) < _votes.size(), slot, _votes.size());
    const auto index = static_cast<std::size_t>(slot);
    _votes[index]    = combine(_votes[index], command);
  }
}

/**
 * Returns the sorted refinement requests of all ranks. Every rank calls this function collectively
 * and returns the same list.
 */
std::vector<peano4::RefinementRequest>
exahype2::RefinementRequests::getRefinementRequests(const peano4::CellConnectivity& cellConnectivity) const {
  return gatherRefinementRequests(getLocalRefinementRequests(cellConnectivity));
}

/**
 * Returns the refinement requests of the cells of this rank in vote order.
 */
std::vector<peano4::RefinementRequest>
exahype2::RefinementRequests::getLocalRefinementRequests(const peano4::CellConnectivity& cellConnectivity) const {
  using Control = peano4::RefinementRequest::RefinementControl;

  std::vector<peano4::RefinementRequest> requests;
  for (const int slot : cellConnectivity.getCells()) {
    if (_votes[static_cast<std::size_t>(slot)] == static_cast<std::uint8_t>(RefinementCommand::Refine)) {
      peano4::RefinementRequest request{};
      request.setRefinementControl(Control::Refine);
      request.level        = cellConnectivity.getCellLevel(slot);
      request.latticeIndex = cellConnectivity.getCellLattice(slot);
      requests.push_back(request);
    }
  }

  const auto eraseVotes = getSortedEraseVotes(_votes, cellConnectivity);
  forEachCompleteSiblingSet(eraseVotes, [&requests, &eraseVotes](const std::size_t firstVote) {
    peano4::RefinementRequest request{};
    request.setRefinementControl(Control::Erase);
    request.level        = eraseVotes[firstVote].level;
    request.latticeIndex = eraseVotes[firstVote].lattice;
    requests.push_back(request);
  });
  return requests;
}

/**
 * Returns the sorted union of the requests of all ranks. Every rank calls this function collectively
 * with its own requests and returns the same list.
 */
std::vector<peano4::RefinementRequest>
exahype2::RefinementRequests::gatherRefinementRequests(std::vector<peano4::RefinementRequest> requests) {
  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const int      numberOfRanks = rank.getNumberOfRanks();
  const MPI_Comm communicator  = rank.getCommunicator();
  if (numberOfRanks > 1) {
    constexpr auto RequestSize = static_cast<int>(sizeof(peano4::RefinementRequest));
    const auto     localBytes  = static_cast<std::size_t>(requests.size()) * RequestSize;
    if (localBytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      tarch::terminateWithError("The refinement requests of one rank must fit into one MPI message.");
    }
    std::vector<int> byteCounts(static_cast<std::size_t>(numberOfRanks), 0);
    const int        localByteCount = static_cast<int>(localBytes);
    tarch::mpi::allgather(&localByteCount, 1, MPI_INT, byteCounts.data(), 1, MPI_INT, communicator);
    std::vector<int> displacements(static_cast<std::size_t>(numberOfRanks), 0);
    std::size_t      totalBytes = 0;
    for (int other = 0; other < numberOfRanks; other++) {
      displacements[static_cast<std::size_t>(other)] = static_cast<int>(totalBytes);
      totalBytes += static_cast<std::size_t>(byteCounts[static_cast<std::size_t>(other)]);
    }
    if (totalBytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      tarch::terminateWithError("The refinement requests of all ranks must fit into one MPI message.");
    }
    std::vector<peano4::RefinementRequest> gathered(totalBytes / RequestSize);
    tarch::mpi::allgatherv(
      requests.data(),
      localByteCount,
      MPI_BYTE,
      gathered.data(),
      byteCounts.data(),
      displacements.data(),
      MPI_BYTE,
      communicator
    );
    requests = std::move(gathered);
  }

  std::sort(requests.begin(), requests.end());
  requests.erase(std::unique(requests.begin(), requests.end()), requests.end());
  return requests;
}
