// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/grid/AutomatonState.h"

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

UNIT_TEST("A default automaton state starts in the root state of the curve and describes its type and curve state") {
  GIVEN("A default automaton state") {
    const peano4::grid::AutomatonState state;

    WHEN("The state writes its description") {
      const std::string description = state.toString();

      THEN("The state starts in the root state of the curve and the description contains its type and curve state") {
        VALIDATE_EQUALS(state.curveState, peano4::sfc::ActiveCurve::RootState);
        VALIDATE(description.find("AutomatonState") != std::string::npos);
        VALIDATE(description.find("CurveState") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A refinement gives every child state the next level, its position, its width and its curve state") {
  GIVEN("A coarse state on level three with the width one and the axis number as its coordinate along every axis") {
    constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
    constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

    peano4::grid::AutomatonState coarseState;
    coarseState.level = 3;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      coarseState.x(axis) = 1.0 * axis;
      coarseState.h(axis) = 1.0;
    }

    WHEN("The coarse state refines") {
      peano4::grid::AutomatonState fineStates[ChildrenPerCell];
      peano4::grid::AutomatonState::refineState(coarseState, fineStates);

      THEN("Every child lies one level down and its width is the coarse width over the subdivision factor") {
        for (int child = 0; child < ChildrenPerCell; child++) {
          VALIDATE_EQUALS(fineStates[child].level, coarseState.level + 1);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_NUMERICAL_EQUALS(fineStates[child].h(axis), coarseState.h(axis) / SubdivisionFactor);
          }
        }
      }

      THEN("The array index of a child is its linearised position in the parent, with the first axis running fastest") {
        for (int child = 0; child < ChildrenPerCell; child++) {
          int remainingIndex = child;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            const double expectedPosition =
              coarseState.x(axis) + (remainingIndex % SubdivisionFactor) * coarseState.h(axis) / SubdivisionFactor;
            remainingIndex /= SubdivisionFactor;
            VALIDATE_NUMERICAL_EQUALS(fineStates[child].x(axis), expectedPosition);
          }
        }
      }

      THEN("Every child gets the curve state that the grammar production of its parent assigns to its position") {
        for (const auto& child : peano4::sfc::ActiveCurve::getChildren(coarseState.curveState)) {
          VALIDATE_EQUALS(fineStates[child.linearisedPosition].curveState, child.state, child.linearisedPosition);
        }
      }
    }
  }
}

UNIT_TEST("An automaton state sent around a ring of ranks as raw bytes arrives unchanged") {
  GIVEN("The successor and the predecessor of every rank in the ring and a state derived from the rank") {
    constexpr int Tag = 4321;

    const auto&    rank          = tarch::mpi::Rank::getInstance();
    const MPI_Comm communicator  = rank.getCommunicator();
    const int      localRank     = rank.getRank();
    const int      numberOfRanks = rank.getNumberOfRanks();

    peano4::grid::AutomatonState sentState;
    sentState.level = 4 + localRank;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      sentState.x(axis) = 0.1 * (axis + 1) + 0.001 * localRank;
      sentState.h(axis) = 0.01 * (axis + 2) + 0.0001 * localRank;
    }
    sentState.curveState = static_cast<std::uint8_t>(localRank % peano4::sfc::ActiveCurve::NumberOfStates);

    const int successor   = (localRank + 1) % numberOfRanks;
    const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

    WHEN("Every rank sends its state to the successor and receives the state of the predecessor") {
      peano4::grid::AutomatonState receivedState;
      const int                    result = tarch::mpi::sendrecv(
        &sentState,
        static_cast<int>(sizeof(peano4::grid::AutomatonState)),
        MPI_BYTE,
        successor,
        Tag,
        &receivedState,
        static_cast<int>(sizeof(peano4::grid::AutomatonState)),
        MPI_BYTE,
        predecessor,
        Tag,
        communicator
      );

      THEN("The transfer succeeds with the level, the geometry and the curve state of the predecessor") {
        VALIDATE_EQUALS(result, MPI_SUCCESS);
        VALIDATE_EQUALS(receivedState.level, 4 + predecessor);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_NUMERICAL_EQUALS(receivedState.x(axis), 0.1 * (axis + 1) + 0.001 * predecessor);
          VALIDATE_NUMERICAL_EQUALS(receivedState.h(axis), 0.01 * (axis + 2) + 0.0001 * predecessor);
        }
        const auto expectedCurveState =
          static_cast<std::uint8_t>(predecessor % peano4::sfc::ActiveCurve::NumberOfStates);
        VALIDATE_EQUALS(receivedState.curveState, expectedCurveState);
      }
    }
  }
}
