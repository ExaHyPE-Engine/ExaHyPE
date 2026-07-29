// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonStateTest.h"

#include "peano4/grid/AutomatonState.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

peano4::grid::tests::AutomatonStateTest::AutomatonStateTest() :
  TestCase("peano4::grid::tests::AutomatonStateTest") {}

void peano4::grid::tests::AutomatonStateTest::run() {
  testMethod(testFlags);
  testMethod(testRefineState);
  testMethod(testSendAndReceiveInARing);
}

void peano4::grid::tests::AutomatonStateTest::testFlags() {
  AutomatonState state;

  state.setInverted(false);
  validateFalse(state.getInverted());
  state.setInverted(true);
  validateTrue(state.getInverted());
  state.setInverted(false);
  validateFalse(state.getInverted());

  for (int d = 0; d < DIMENSIONS; d++) {
    state.setEvenFlag(d, false);
    validateFalse(state.getEvenFlag(d));
  }

  // The even flag of one axis and the inverted flag share a word, so setting a pattern on all of
  // them and reading it back catches a mask that reaches into a neighbouring bit.
  for (int d = 0; d < DIMENSIONS; d++) {
    const bool expectedEvenFlag = (d % 2) == 1;
    state.setEvenFlag(d, expectedEvenFlag);
    validateEquals(state.getEvenFlag(d), expectedEvenFlag);
  }
  state.setInverted(true);
  validateTrue(state.getInverted());
  for (int d = 0; d < DIMENSIONS; d++) {
    const bool expectedEvenFlag = (d % 2) == 1;
    validateEquals(state.getEvenFlag(d), expectedEvenFlag);
  }

  for (int d = 0; d < DIMENSIONS; d++) {
    const bool flagBeforeFlip = state.getEvenFlag(d);
    state.flipEvenFlag(d);
    validateEquals(state.getEvenFlag(d), not flagBeforeFlip);
  }
  validateTrue(state.getInverted());

  const std::string description = state.toString();
  validateTrue(description.find("AutomatonState") != std::string::npos);
  validateTrue(description.find("Inverted=true") != std::string::npos);
}

void peano4::grid::tests::AutomatonStateTest::testRefineState() {
  AutomatonState coarseState;
  coarseState.level = 3;
  for (int d = 0; d < DIMENSIONS; d++) {
    coarseState.x(d) = 1.0 * d;
    coarseState.h(d) = 1.0;
  }

  AutomatonState fineStates[THREE_POWER_D];
  AutomatonState::refineState(coarseState, fineStates);

  for (int i = 0; i < THREE_POWER_D; i++) {
    validateEquals(fineStates[i].level, coarseState.level + 1);
    for (int d = 0; d < DIMENSIONS; d++) {
      validateNumericalEquals(fineStates[i].h(d), coarseState.h(d) / 3.0);
    }
  }

  // A child's array slot is its position within the parent, with the first axis running fastest.
  // The children hence tile the parent, and a caller can index them by position.
  for (int i = 0; i < THREE_POWER_D; i++) {
    int remainingIndex = i;
    for (int d = 0; d < DIMENSIONS; d++) {
      const double expectedPosition = coarseState.x(d) + (remainingIndex % 3) * coarseState.h(d) / 3.0;
      remainingIndex /= 3;
      validateNumericalEquals(fineStates[i].x(d), expectedPosition);
    }
  }
}

void peano4::grid::tests::AutomatonStateTest::testSendAndReceiveInARing() {
  constexpr int Tag = 4321;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();

  // Every field depends on the rank, so a receiver can predict what its predecessor sent.
  AutomatonState sentState;
  sentState.level = 4 + localRank;
  for (int d = 0; d < DIMENSIONS; d++) {
    sentState.x(d) = 0.1 * (d + 1) + 0.001 * localRank;
    sentState.h(d) = 0.01 * (d + 2) + 0.0001 * localRank;
    sentState.setEvenFlag(d, ((d + localRank) % 2) == 1);
  }
  sentState.setInverted((localRank % 2) == 1);
  for (int i = 0; i < TWO_TIMES_D; i++) {
    sentState.accessNumber[i] = static_cast<std::int8_t>((i % 5) - 2 + localRank);
  }

  const int successor   = (localRank + 1) % numberOfRanks;
  const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

  AutomatonState receivedState;
  const int      result = tarch::mpi::sendrecv(
    &sentState,
    static_cast<int>(sizeof(AutomatonState)),
    MPI_BYTE,
    successor,
    Tag,
    &receivedState,
    static_cast<int>(sizeof(AutomatonState)),
    MPI_BYTE,
    predecessor,
    Tag,
    communicator
  );
  validateEquals(result, MPI_SUCCESS);

  validateEquals(receivedState.level, 4 + predecessor);
  for (int d = 0; d < DIMENSIONS; d++) {
    // The expected flag needs its own variable because validateEquals streams the expression, and the
    // comparison would bind to the stream operator.
    const bool expectedEvenFlag = ((d + predecessor) % 2) == 1;
    validateNumericalEquals(receivedState.x(d), 0.1 * (d + 1) + 0.001 * predecessor);
    validateNumericalEquals(receivedState.h(d), 0.01 * (d + 2) + 0.0001 * predecessor);
    validateEquals(receivedState.getEvenFlag(d), expectedEvenFlag);
  }

  const bool expectedInverted = (predecessor % 2) == 1;
  validateEquals(receivedState.getInverted(), expectedInverted);

  for (int i = 0; i < TWO_TIMES_D; i++) {
    validateEquals(receivedState.accessNumber[i], static_cast<std::int8_t>((i % 5) - 2 + predecessor));
  }
}
