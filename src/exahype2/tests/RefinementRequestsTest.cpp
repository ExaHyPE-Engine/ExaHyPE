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

  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  int getCellAt(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    return peano4::Spacetree::getInstance().getCellAt(Depth, lattice);
  }

  bool ownsEveryChildOf(const tarch::la::Vector<DIMENSIONS, int>& parentLattice) {
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
   * Returns the number of parents whose children all lie on one rank, summed over the ranks. A
   * parent with children on two ranks never erases.
   */
  int getNumberOfWhollyOwnedParents() {
    int wholeParents = 0;
    peano4::dForEach(peano4::SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& parentLattice) {
      if (ownsEveryChildOf(parentLattice)) {
        wholeParents++;
      }
    });
    return peano4::tests::getGlobalSumOf(wholeParents);
  }

  /**
   * Returns whether one rank owns the origin cell and all its siblings.
   */
  bool isOriginParentWhollyOwned() { return peano4::tests::getGlobalSumOf(ownsEveryChildOf(Origin) ? 1 : 0) == 1; }

  /**
   * Votes of one solver, one per slot of the partition.
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

  bool containsRequestAt(
    const std::vector<peano4::RefinementRequest>& requests,
    const int                                     level,
    const tarch::la::Vector<DIMENSIONS, int>&     lattice
  ) {
    return std::ranges::any_of(requests, [&](const peano4::RefinementRequest& request) {
      return request.level == level and request.latticeIndex == lattice;
    });
  }
} // namespace

exahype2::tests::RefinementRequestsTest::RefinementRequestsTest() :
  tarch::tests::TestCase("exahype2::tests::RefinementRequestsTest") {}

void
exahype2::tests::RefinementRequestsTest::run() {
  testMethod(testKeepVotesRaiseNoRequest);
  testMethod(testARefineVoteBecomesARequestOnEveryRank);
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
exahype2::tests::RefinementRequestsTest::testARefineVoteBecomesARequestOnEveryRank() {
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
 * Every parent whose children all vote Erase on one rank erases. One child that votes Keep keeps its parent.
 */
void
exahype2::tests::RefinementRequestsTest::testAnEraseStandsOnceEveryChildVotes() {
  const auto& partition         = buildGrid();
  const int   wholeParents      = getNumberOfWhollyOwnedParents();
  const bool  originParentWhole = isOriginParentWhollyOwned();
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
  validateEquals(containsRequestAt(refinementRequests, Depth - 1, Origin), originParentWhole);

  votes.voteOnTheOriginCell(exahype2::RefinementCommand::Keep);
  requests.clear(partition.getNumberOfSlots());
  requests.merge(votes.commands, partition.getCells());
  const auto requestsWithOneKeep = requests.getRefinementRequests(partition);
  validateEquals(static_cast<int>(requestsWithOneKeep.size()), wholeParents - (originParentWhole ? 1 : 0));
  validate(not containsRequestAt(requestsWithOneKeep, Depth - 1, Origin));
}

void
exahype2::tests::RefinementRequestsTest::testRefineWinsAndKeepBeatsErase() {
  const auto& partition = buildGrid();
  Votes       eraseVotes(partition);
  eraseVotes.voteOnEveryCell(partition, exahype2::RefinementCommand::Erase);
  const Votes keepVotes(partition);
  Votes       refineVote(partition);
  refineVote.voteOnEveryCell(partition, exahype2::RefinementCommand::Erase);
  refineVote.voteOnTheOriginCell(exahype2::RefinementCommand::Refine);
  const int  wholeParents      = getNumberOfWhollyOwnedParents();
  const bool originParentWhole = isOriginParentWhollyOwned();

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
  validate(containsRequestAt(refinementRequests, Depth, Origin));
  validate(not containsRequestAt(refinementRequests, Depth - 1, Origin));
}
