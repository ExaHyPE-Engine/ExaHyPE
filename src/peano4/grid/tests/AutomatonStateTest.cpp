// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonStateTest.h"

#include "peano4/grid/AutomatonState.h"
#include "tarch/mpi/mpi.h"

peano4::grid::tests::AutomatonStateTest::AutomatonStateTest() :
  TestCase("peano4::grid::tests::AutomatonStateTest") {}

void peano4::grid::tests::AutomatonStateTest::run() {
  testMethod(testFlags);
  testMethod(testMPIDatatype);
}

void peano4::grid::tests::AutomatonStateTest::testFlags() {
  AutomatonState state;

  state.setInverted(false);
  validateFalse(state.getInverted());
  state.setInverted(true);
  validateTrue(state.getInverted());
  state.setInverted(false);
  validateFalse(state.getInverted());

  for (int d = 0; d < DIMENSIONS; ++d) {
    state.setEvenFlag(d, false);
    validateFalse(state.getEvenFlag(d));
  }

  // Set a pattern: for DIMENSIONS >= 2 set [0]=false, [1]=true; for others toggle every second bit
  for (int d = 0; d < DIMENSIONS; ++d) {
    bool v = (d % 2) == 1; // 0101...
    state.setEvenFlag(d, v);
    validateEquals(state.getEvenFlag(d), v);
  }

  // Flip a couple of bits and check
  if constexpr (DIMENSIONS >= 1) {
    const bool before = state.getEvenFlag(0);
    state.flipEvenFlag(0);
    validateEquals(state.getEvenFlag(0), not before);
  }
  if constexpr (DIMENSIONS >= 2) {
    const bool before = state.getEvenFlag(1);
    state.flipEvenFlag(1);
    validateEquals(state.getEvenFlag(1), not before);
  }

  // Parity check equivalent to:
  // result = (count(other-axis even flags) % 2 == 0) XOR inverted
  // Test a few axes for robustness
  auto parityRef = [&](const AutomatonState& st, int axis) {
    int cnt = 0;
    for (int d = 0; d < DIMENSIONS; ++d) {
      if (d == axis)
        continue;
      if (st.getEvenFlag(d))
        ++cnt;
    }
    bool parityEven = (cnt % 2) == 0;
    return parityEven ^ st.getInverted();
  };
  auto expectedParity = [&](int axis, bool inverted) {
    int cnt = 0;
    for (int d = 0; d < DIMENSIONS; ++d) {
      if (d != axis and state.getEvenFlag(d))
        ++cnt;
    }
    return ((cnt & 1) == 0) ^ inverted;
  };

  // Check for inverted=false and multiple axes
  state.setInverted(false);
  for (int axis = 0; axis < DIMENSIONS; ++axis) {
    validateTrue(parityRef(state, axis) == expectedParity(axis, false));
  }

  // Check again for inverted=true
  state.setInverted(true);
  for (int axis = 0; axis < DIMENSIONS; ++axis) {
    validateTrue(parityRef(state, axis) == expectedParity(axis, true));
  }

  // std::cout << state << std::endl;
}

void peano4::grid::tests::AutomatonStateTest::testMPIDatatype() {
#ifdef PEANO_WITH_MPI
  auto&     rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm  comm          = rank.getCommunicator();
  const int me            = rank.getRank();
  const int numberOfRanks = rank.getNumberOfRanks();
  const int tag           = 4321;

  AutomatonState sendS;
  sendS.level = 4 + me;

  // Fill x/h with recognizable values (index-based)
  for (int d = 0; d < DIMENSIONS; ++d) {
    sendS.x(d) = 0.1 * (d + 1) + 0.001 * me;   // e.g. 0.101, 0.201, ... for rank 1
    sendS.h(d) = 0.01 * (d + 2) + 0.0001 * me; // e.g. 0.0201, ...
  }

  // Flags: choose inverted based on rank parity and set even flags pattern
  sendS.setInverted((me % 2) == 1);
  for (int d = 0; d < DIMENSIONS; ++d) {
    // Pattern depends on rank so received flags can be predicted:
    sendS.setEvenFlag(d, ((d + me) % 2) == 1);
  }

  // Access numbers: produce small values dependent on rank
  for (int i = 0; i < TWO_TIMES_D; ++i) {
    int v                 = (i % 5) - 2 + me; // Small values shifted by rank
    sendS.accessNumber[i] = v;
  }

  AutomatonState recvS;

  // TODO: Test non-blocking communication as well
  if (numberOfRanks == 1) {
    tarch::mpi::sendrecv( // TODO: Capture result for error checking
        &sendS,
        1,
        AutomatonState::Datatype,
        me,
        tag,
        &recvS,
        1,
        AutomatonState::Datatype,
        me,
        tag,
        comm,
        MPI_STATUS_IGNORE);
    recvS.senderDestinationRank = me;
    // AutomatonState::send(sendS, me, tag, comm);
    // AutomatonState::receive(recvS, me, tag, comm);
  } else {
    const int dest = (me + 1) % numberOfRanks;
    const int src  = (me - 1 + numberOfRanks) % numberOfRanks;

    if ((me % 2) == 0) {
      // Even ranks send then receive
      AutomatonState::send(sendS, dest, tag, comm);
      AutomatonState::receive(recvS, src, tag, comm);
    } else {
      // Odd ranks receive then send
      AutomatonState::receive(recvS, src, tag, comm);
      AutomatonState::send(sendS, dest, tag, comm);
    }
  }

  const int expectedSenderRank = (numberOfRanks == 1) ? me : ((me - 1 + numberOfRanks) % numberOfRanks);
  validateEquals(recvS.level, 4 + expectedSenderRank);

  for (int d = 0; d < DIMENSIONS; ++d) {
    const double expectedX = 0.1 * (d + 1) + 0.001 * expectedSenderRank;
    const double expectedH = 0.01 * (d + 2) + 0.0001 * expectedSenderRank;
    validateTrue(std::fabs(recvS.x(d) - expectedX) < 1e-12);
    validateTrue(std::fabs(recvS.h(d) - expectedH) < 1e-12);
    const bool expectedEven = ((d + expectedSenderRank) % 2) == 1;
    validateEquals(recvS.getEvenFlag(d), expectedEven);
  }

  validateEquals(recvS.getInverted(), ((expectedSenderRank % 2) == 1));

  for (int i = 0; i < TWO_TIMES_D; ++i) {
    const auto got = recvS.accessNumber[i];
    const auto exp = ((i % 5) - 2) + expectedSenderRank;
    validateEquals(got, exp);
  }
#endif
}
