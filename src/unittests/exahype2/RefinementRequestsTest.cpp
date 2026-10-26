// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/RefinementRequests.h"

#include "exahype2/RefinementCommand.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Loop.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Spacetree.h"
#include "tarch/Span.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  constexpr int Depth = 2;

  const tarch::la::Vector<DIMENSIONS, int> Origin(0);

  using Control = peano4::RefinementRequest::RefinementControl;

  const peano4::CellConnectivity& buildGrid() {
    peano4::unittests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
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
    return peano4::unittests::getGlobalSumOf(wholeParents);
  }

  /**
   * Returns whether one rank owns every child of the parent of the origin cell.
   */
  bool isOriginParentWhollyOwned() { return peano4::unittests::getGlobalSumOf(ownsEveryChildOf(Origin) ? 1 : 0) == 1; }

  /**
   * Votes of one solver, one per slot of the cell connectivity.
   */
  struct Votes final {
    std::vector<exahype2::RefinementCommand> commands;

    explicit Votes(const peano4::CellConnectivity& cellConnectivity) :
      commands(static_cast<std::size_t>(cellConnectivity.getNumberOfSlots()), exahype2::RefinementCommand::Keep) {}

    void voteOnEveryCell(const peano4::CellConnectivity& cellConnectivity, const exahype2::RefinementCommand command) {
      for (const int cell : cellConnectivity.getCells()) {
        commands[static_cast<std::size_t>(cell)] = command;
      }
    }

    /**
     * Casts the vote on the cell at the lattice origin. One rank owns this cell.
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

UNIT_TEST("Keep votes on every cell leave the refinement requests empty") {
  GIVEN("A regular grid over every rank and a keep vote on every cell") {
    const auto& cellConnectivity = buildGrid();
    const Votes votes(cellConnectivity);

    WHEN("The votes merge and the ranks derive the refinement requests") {
      exahype2::RefinementRequests requests;
      requests.clear(cellConnectivity.getNumberOfSlots());
      requests.merge(votes.commands, cellConnectivity.getCells());
      const auto refinementRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("The refinement requests stay empty") { VALIDATE(refinementRequests.empty()); }
    }
  }
}

UNIT_TEST("A refine vote becomes a refine request of its cell on every rank") {
  GIVEN("A regular grid over every rank and a refine vote on the origin cell") {
    const auto& cellConnectivity = buildGrid();
    Votes       votes(cellConnectivity);
    votes.voteOnTheOriginCell(exahype2::RefinementCommand::Refine);

    WHEN("The votes merge and the ranks derive the refinement requests") {
      exahype2::RefinementRequests requests;
      requests.clear(cellConnectivity.getNumberOfSlots());
      requests.merge(votes.commands, cellConnectivity.getCells());
      const auto refinementRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("Every rank receives one request, the refine request of the origin cell") {
        VALIDATE_EQUALS(refinementRequests.size(), std::size_t{1});
        VALIDATE_EQUALS(
          static_cast<int>(refinementRequests[0].getRefinementControl()),
          static_cast<int>(Control::Refine)
        );
        VALIDATE_EQUALS(refinementRequests[0].level, Depth);
        VALIDATE_EQUALS(refinementRequests[0].latticeIndex, Origin);
      }
    }
  }
}

UNIT_TEST("A parent erases once all its children vote for it on one rank and a keep of one child blocks the erase") {
  GIVEN("A regular grid over every rank, the wholly owned parents and an erase vote on every cell") {
    const auto& cellConnectivity  = buildGrid();
    const int   wholeParents      = getNumberOfWhollyOwnedParents();
    const bool  originParentWhole = isOriginParentWhollyOwned();
    Votes       votes(cellConnectivity);
    votes.voteOnEveryCell(cellConnectivity, exahype2::RefinementCommand::Erase);

    WHEN("The votes merge and the ranks derive the refinement requests") {
      exahype2::RefinementRequests requests;
      requests.clear(cellConnectivity.getNumberOfSlots());
      requests.merge(votes.commands, cellConnectivity.getCells());
      const auto refinementRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("The erase requests name the wholly owned parents one level up, at least one on a single rank") {
        VALIDATE(wholeParents >= 1 or tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);
        VALIDATE_EQUALS(static_cast<int>(refinementRequests.size()), wholeParents);
        VALIDATE_EQUALS(getNumberOfRequestsWith(refinementRequests, Control::Erase), wholeParents);
        for (const auto& request : refinementRequests) {
          VALIDATE_EQUALS(request.level, Depth - 1);
        }
        VALIDATE_EQUALS(containsRequestAt(refinementRequests, Depth - 1, Origin), originParentWhole);
      }
    }

    WHEN("The origin cell votes keep, the votes merge and the ranks derive the refinement requests") {
      votes.voteOnTheOriginCell(exahype2::RefinementCommand::Keep);
      exahype2::RefinementRequests requests;
      requests.clear(cellConnectivity.getNumberOfSlots());
      requests.merge(votes.commands, cellConnectivity.getCells());
      const auto requestsWithOneKeep = requests.getRefinementRequests(cellConnectivity);

      THEN("The parent of the origin cell loses its erase request and every other wholly owned parent keeps it") {
        VALIDATE_EQUALS(static_cast<int>(requestsWithOneKeep.size()), wholeParents - (originParentWhole ? 1 : 0));
        VALIDATE(not containsRequestAt(requestsWithOneKeep, Depth - 1, Origin));
      }
    }
  }
}

UNIT_TEST("A keep of a second solver blocks every erase and a refine names its cell and blocks the parent's erase") {
  GIVEN("A regular grid over every rank, the wholly owned parents and the erase votes of one solver") {
    const auto& cellConnectivity  = buildGrid();
    const int   wholeParents      = getNumberOfWhollyOwnedParents();
    const bool  originParentWhole = isOriginParentWhollyOwned();
    Votes       eraseVotes(cellConnectivity);
    eraseVotes.voteOnEveryCell(cellConnectivity, exahype2::RefinementCommand::Erase);

    WHEN("The keep votes of a second solver merge after the erase votes") {
      const Votes                  keepVotes(cellConnectivity);
      exahype2::RefinementRequests keepAgainstErase;
      keepAgainstErase.clear(cellConnectivity.getNumberOfSlots());
      keepAgainstErase.merge(eraseVotes.commands, cellConnectivity.getCells());
      keepAgainstErase.merge(keepVotes.commands, cellConnectivity.getCells());
      const auto refinementRequests = keepAgainstErase.getRefinementRequests(cellConnectivity);

      THEN("The refinement requests stay empty") { VALIDATE(refinementRequests.empty()); }
    }

    WHEN("A keep command on every cell merges once before and once after the erase votes") {
      std::vector<std::vector<peano4::RefinementRequest>> refinementRequestsOfEachOrder;
      for (const bool keepFirst : {false, true}) {
        exahype2::RefinementRequests keepAgainstErase;
        keepAgainstErase.clear(cellConnectivity.getNumberOfSlots());
        if (keepFirst) {
          keepAgainstErase.merge(exahype2::RefinementCommand::Keep, cellConnectivity.getCells());
        }
        keepAgainstErase.merge(eraseVotes.commands, cellConnectivity.getCells());
        if (not keepFirst) {
          keepAgainstErase.merge(exahype2::RefinementCommand::Keep, cellConnectivity.getCells());
        }
        refinementRequestsOfEachOrder.push_back(keepAgainstErase.getRefinementRequests(cellConnectivity));
      }

      THEN("The refinement requests of both orders stay empty") {
        for (const auto& refinementRequests : refinementRequestsOfEachOrder) {
          VALIDATE(refinementRequests.empty());
        }
      }
    }

    WHEN("A keep command on an empty cell list merges after the erase votes") {
      exahype2::RefinementRequests keepAgainstErase;
      keepAgainstErase.clear(cellConnectivity.getNumberOfSlots());
      keepAgainstErase.merge(eraseVotes.commands, cellConnectivity.getCells());
      keepAgainstErase.merge(exahype2::RefinementCommand::Keep, tarch::Span<const int>{});
      const auto refinementRequests = keepAgainstErase.getRefinementRequests(cellConnectivity);

      THEN("Every wholly owned parent keeps its erase request") {
        VALIDATE_EQUALS(getNumberOfRequestsWith(refinementRequests, Control::Erase), wholeParents);
      }
    }

    WHEN("A second solver votes erase on every cell and refine on the origin cell and its votes merge") {
      Votes refineVote(cellConnectivity);
      refineVote.voteOnEveryCell(cellConnectivity, exahype2::RefinementCommand::Erase);
      refineVote.voteOnTheOriginCell(exahype2::RefinementCommand::Refine);
      exahype2::RefinementRequests refineAgainstErase;
      refineAgainstErase.clear(cellConnectivity.getNumberOfSlots());
      refineAgainstErase.merge(eraseVotes.commands, cellConnectivity.getCells());
      refineAgainstErase.merge(refineVote.commands, cellConnectivity.getCells());
      const auto refinementRequests = refineAgainstErase.getRefinementRequests(cellConnectivity);

      THEN("The origin cell receives the one refine request and only its parent loses the erase request") {
        VALIDATE_EQUALS(getNumberOfRequestsWith(refinementRequests, Control::Refine), 1);
        VALIDATE_EQUALS(
          getNumberOfRequestsWith(refinementRequests, Control::Erase),
          wholeParents - (originParentWhole ? 1 : 0)
        );
        VALIDATE(containsRequestAt(refinementRequests, Depth, Origin));
        VALIDATE(not containsRequestAt(refinementRequests, Depth - 1, Origin));
      }
    }
  }
}
