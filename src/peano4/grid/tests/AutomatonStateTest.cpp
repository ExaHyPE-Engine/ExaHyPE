// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonStateTest.h"

#include "peano4/grid/AutomatonState.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

peano4::grid::tests::AutomatonStateTest::AutomatonStateTest() :
  TestCase("peano4::grid::tests::AutomatonStateTest") {}

void
peano4::grid::tests::AutomatonStateTest::run() {
  testMethod(testCurveState);
  testMethod(testRefineState);
  testMethod(testSendAndReceiveInARing);
}

void
peano4::grid::tests::AutomatonStateTest::testCurveState() {
  const peano4::grid::AutomatonState state;
  validateEquals(state.curveState, peano4::sfc::ActiveCurve::RootState);

  const std::string description = state.toString();
  validateTrue(description.find("AutomatonState") != std::string::npos);
  validateTrue(description.find("CurveState") != std::string::npos);
}

void
peano4::grid::tests::AutomatonStateTest::testRefineState() {
  peano4::grid::AutomatonState coarseState;
  coarseState.level = 3;
  for (int d = 0; d < DIMENSIONS; d++) {
    coarseState.x(d) = 1.0 * d;
    coarseState.h(d) = 1.0;
  }

  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  peano4::grid::AutomatonState fineStates[ChildrenPerCell];
  peano4::grid::AutomatonState::refineState(coarseState, fineStates);

  for (int i = 0; i < ChildrenPerCell; i++) {
    validateEquals(fineStates[i].level, coarseState.level + 1);
    for (int d = 0; d < DIMENSIONS; d++) {
      validateNumericalEquals(fineStates[i].h(d), coarseState.h(d) / SubdivisionFactor);
    }
  }

  // A child's array slot is its position within the parent, with the first axis running fastest.
  for (int i = 0; i < ChildrenPerCell; i++) {
    int remainingIndex = i;
    for (int d = 0; d < DIMENSIONS; d++) {
      const double expectedPosition =
        coarseState.x(d) + (remainingIndex % SubdivisionFactor) * coarseState.h(d) / SubdivisionFactor;
      remainingIndex /= SubdivisionFactor;
      validateNumericalEquals(fineStates[i].x(d), expectedPosition);
    }
  }

  // Every child carries the curve state its parent's production assigns to its position.
  for (const auto& child : peano4::sfc::ActiveCurve::getChildren(coarseState.curveState)) {
    validateEqualsWithParams1(fineStates[child.linearisedPosition].curveState, child.state, child.linearisedPosition);
  }
}

void
peano4::grid::tests::AutomatonStateTest::testSendAndReceiveInARing() {
  constexpr int Tag = 4321;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();

  peano4::grid::AutomatonState sentState;
  sentState.level = 4 + localRank;
  for (int d = 0; d < DIMENSIONS; d++) {
    sentState.x(d) = 0.1 * (d + 1) + 0.001 * localRank;
    sentState.h(d) = 0.01 * (d + 2) + 0.0001 * localRank;
  }
  sentState.curveState = static_cast<std::uint8_t>(localRank % peano4::sfc::ActiveCurve::NumberOfStates);

  const int successor   = (localRank + 1) % numberOfRanks;
  const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

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
  validateEquals(result, MPI_SUCCESS);

  validateEquals(receivedState.level, 4 + predecessor);
  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(receivedState.x(d), 0.1 * (d + 1) + 0.001 * predecessor);
    validateNumericalEquals(receivedState.h(d), 0.01 * (d + 2) + 0.0001 * predecessor);
  }
  const auto expectedCurveState = static_cast<std::uint8_t>(predecessor % peano4::sfc::ActiveCurve::NumberOfStates);
  validateEquals(receivedState.curveState, expectedCurveState);
}
