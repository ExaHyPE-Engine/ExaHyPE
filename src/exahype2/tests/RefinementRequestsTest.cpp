// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementRequestsTest.h"

#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/Span.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int Depth = 2;

  const tarch::la::Vector<DIMENSIONS, int> Origin(0);

  using Control = peano4::RefinementRequest::RefinementControl;

  /**
   * Builds a regular grid of Depth levels partitioned over the ranks and returns the partition of this rank.
   */
  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  int getCellAt(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    return peano4::Spacetree::getInstance().getCellAt(Depth, lattice);
  }

  /**
   * Returns whether the rank holds every child cell of the parent cell at the given lattice position.
   */
  bool holdsEveryChildOf(const tarch::la::Vector<DIMENSIONS, int>& parentLattice) {
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      const auto                         childPosition = peano4::dDelinearised(child, peano4::SubdivisionFactor);
      tarch::la::Vector<DIMENSIONS, int> childLattice;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childLattice(axis) = peano4::SubdivisionFactor * parentLattice(axis) + childPosition(axis);
      }
      if (getCellAt(childLattice) < 0) {
        return false;
      }
    }
    return true;
  }

  /**
   * Counts over all ranks the parents whose children all lie on one rank; the base grid is
   * partitioned by cells, so a parent with children on two ranks never erases.
   */
  int getNumberOfParentsHeldWhole() {
    int wholeParents = 0;
    peano4::dForEach(peano4::SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& parentLattice) {
      if (holdsEveryChildOf(parentLattice)) {
        wholeParents++;
      }
    });
    return peano4::tests::getGlobalSumOf(wholeParents);
  }

  /**
   * Returns whether the rank that owns the origin cell holds every sibling of it.
   */
  bool originParentIsHeldWhole() { return peano4::tests::getGlobalSumOf(holdsEveryChildOf(Origin) ? 1 : 0) == 1; }

  /**
   * Holds the votes of one solver, one per slot of the partition.
   */
  struct Votes final {
    std::vector<exahype2::RefinementCommand> commands;

    explicit Votes(const peano4::Partition& partition) :
      commands(static_cast<std::size_t>(partition.getNumberOfSlots()), exahype2::RefinementCommand::Keep) {}

    void voteOnEveryCell(const peano4::Partition& partition, const exahype2::RefinementCommand command) {
      for (const int cell : partition.getCells()) {
        commands[static_cast<std::size_t>(cell)] = command;
      }
    }

    /**
     * Casts the vote on the cell at the lattice origin, which one rank owns.
     */
    void voteOnTheOriginCell(const exahype2::RefinementCommand command) {
      const int cell = getCellAt(Origin);
      if (cell >= 0) {
        commands[static_cast<std::size_t>(cell)] = command;
      }
    }
  };

  int getNumberOfRequestsWith(const std::vector<peano4::RefinementRequest>& requests, const Control control) {
    int count = 0;
    for (const auto& request : requests) {
      if (request.getRefinementControl() == control) {
        count++;
      }
    }
    return count;
  }

  bool holdsARequestAt(
    const std::vector<peano4::RefinementRequest>& requests,
    const int                                     level,
    const tarch::la::Vector<DIMENSIONS, int>&     lattice
  ) {
    for (const auto& request : requests) {
      if (request.level == level and request.latticeIndex == lattice) {
        return true;
      }
    }
    return false;
  }
} // namespace

exahype2::tests::RefinementRequestsTest::RefinementRequestsTest() :
  tarch::tests::TestCase("exahype2::tests::RefinementRequestsTest") {}

void
exahype2::tests::RefinementRequestsTest::run() {
  testMethod(testKeepVotesRaiseNoRequest);
  testMethod(testARefineVoteNamesTheCellOnEveryRank);
  testMethod(testAnEraseStandsOnceEveryChildVotes);
  testMethod(testRefineWinsAndKeepBeatsErase);
}

void
exahype2::tests::RefinementRequestsTest::testKeepVotesRaiseNoRequest() {
  const auto&                  partition = buildGrid();
  const Votes                  votes(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  requests.merge(votes.commands, partition.getCells());
  validate(requests.getRefinementRequests(partition).empty());
}

void
exahype2::tests::RefinementRequestsTest::testARefineVoteNamesTheCellOnEveryRank() {
  const auto& partition = buildGrid();
  Votes       votes(partition);
  votes.voteOnTheOriginCell(exahype2::RefinementCommand::Refine);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  requests.merge(votes.commands, partition.getCells());

  const auto refinementRequests = requests.getRefinementRequests(partition);
  validateEquals(refinementRequests.size(), std::size_t{1});
  validateEquals(static_cast<int>(refinementRequests[0].getRefinementControl()), static_cast<int>(Control::Refine));
  validateEquals(refinementRequests[0].level, Depth);
  validateEquals(refinementRequests[0].latticeIndex, Origin);
}

/**
 * Every parent whose children all vote on one rank erases; the parent of one child that keeps stays.
 */
void
exahype2::tests::RefinementRequestsTest::testAnEraseStandsOnceEveryChildVotes() {
  const auto& partition         = buildGrid();
  const int   wholeParents      = getNumberOfParentsHeldWhole();
  const bool  originParentWhole = originParentIsHeldWhole();
  validate(wholeParents >= 1 or tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);

  Votes votes(partition);
  votes.voteOnEveryCell(partition, exahype2::RefinementCommand::Erase);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  requests.merge(votes.commands, partition.getCells());

  const auto refinementRequests = requests.getRefinementRequests(partition);
  validateEquals(static_cast<int>(refinementRequests.size()), wholeParents);
  validateEquals(getNumberOfRequestsWith(refinementRequests, Control::Erase), wholeParents);
  for (const auto& request : refinementRequests) {
    validateEquals(request.level, Depth - 1);
  }
  validateEquals(holdsARequestAt(refinementRequests, Depth - 1, Origin), originParentWhole);

  votes.voteOnTheOriginCell(exahype2::RefinementCommand::Keep);
  requests.clear(partition.getNumberOfSlots());
  requests.merge(votes.commands, partition.getCells());
  const auto requestsWithOneKeep = requests.getRefinementRequests(partition);
  validateEquals(static_cast<int>(requestsWithOneKeep.size()), wholeParents - (originParentWhole ? 1 : 0));
  validate(not holdsARequestAt(requestsWithOneKeep, Depth - 1, Origin));
}

/**
 * A keep of a second solver blocks every erase, and a refine of one solver names its cell and blocks the parent's
 * erase.
 */
void
exahype2::tests::RefinementRequestsTest::testRefineWinsAndKeepBeatsErase() {
  const auto& partition = buildGrid();
  Votes       eraseVotes(partition);
  eraseVotes.voteOnEveryCell(partition, exahype2::RefinementCommand::Erase);
  const Votes keepVotes(partition);
  Votes       refineVote(partition);
  refineVote.voteOnEveryCell(partition, exahype2::RefinementCommand::Erase);
  refineVote.voteOnTheOriginCell(exahype2::RefinementCommand::Refine);
  const int  wholeParents      = getNumberOfParentsHeldWhole();
  const bool originParentWhole = originParentIsHeldWhole();

  exahype2::RefinementRequests keepAgainstErase;
  keepAgainstErase.clear(partition.getNumberOfSlots());
  keepAgainstErase.merge(eraseVotes.commands, partition.getCells());
  keepAgainstErase.merge(keepVotes.commands, partition.getCells());
  validate(keepAgainstErase.getRefinementRequests(partition).empty());

  for (const bool keepFirst : {false, true}) {
    keepAgainstErase.clear(partition.getNumberOfSlots());
    if (keepFirst) {
      keepAgainstErase.merge(exahype2::RefinementCommand::Keep, partition.getCells());
    }
    keepAgainstErase.merge(eraseVotes.commands, partition.getCells());
    if (not keepFirst) {
      keepAgainstErase.merge(exahype2::RefinementCommand::Keep, partition.getCells());
    }
    validate(keepAgainstErase.getRefinementRequests(partition).empty());
  }

  keepAgainstErase.clear(partition.getNumberOfSlots());
  keepAgainstErase.merge(eraseVotes.commands, partition.getCells());
  keepAgainstErase.merge(exahype2::RefinementCommand::Keep, tarch::Span<const int>{});
  validateEquals(
    getNumberOfRequestsWith(keepAgainstErase.getRefinementRequests(partition), Control::Erase),
    wholeParents
  );

  exahype2::RefinementRequests refineAgainstErase;
  refineAgainstErase.clear(partition.getNumberOfSlots());
  refineAgainstErase.merge(eraseVotes.commands, partition.getCells());
  refineAgainstErase.merge(refineVote.commands, partition.getCells());
  const auto refinementRequests = refineAgainstErase.getRefinementRequests(partition);
  validateEquals(getNumberOfRequestsWith(refinementRequests, Control::Refine), 1);
  validateEquals(
    getNumberOfRequestsWith(refinementRequests, Control::Erase),
    wholeParents - (originParentWhole ? 1 : 0)
  );
  validate(holdsARequestAt(refinementRequests, Depth, Origin));
  validate(not holdsARequestAt(refinementRequests, Depth - 1, Origin));
}
