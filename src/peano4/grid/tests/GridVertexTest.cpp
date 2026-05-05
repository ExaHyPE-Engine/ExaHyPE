// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertexTest.h"

#include "peano4/grid/GridVertex.h"

peano4::grid::tests::GridVertexTest::GridVertexTest():
  TestCase("peano4::grid::tests::GridVertexTest") {}

void peano4::grid::tests::GridVertexTest::run() {
  testMethod(testFlagsAndState);
  testMethod(testCreateGridVertex);
  testMethod(testMPIDatatype);
}

void peano4::grid::tests::GridVertexTest::testFlagsAndState() {
  GridVertex vertex;
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::New));

  // Round-trip state set/get
  vertex.setState(GridVertex::State::Refined);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::Refined));
  vertex.setState(GridVertex::State::EraseTriggered);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::EraseTriggered));

  // Flags: set, clear, toggle
  // Start with all flags cleared
  vertex.flags = 0;
  for (int b = 0; b < 8; ++b) {
    // none set
    validateFalse(vertex.getFlag(static_cast<std::uint8_t>(1u << b)));
  }

  // Set some flags
  vertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  vertex.setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);
  validateTrue(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  // Clear one and check
  vertex.clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  validateFalse(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
  validateTrue(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, false);
  validateFalse(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));

  // std::cout << vertex << std::endl;
}

void peano4::grid::tests::GridVertexTest::testCreateGridVertex() {
  std::array<int, TWO_POWER_D> adj{};
  adj.fill(7);
  tarch::la::Vector<DIMENSIONS, double> x(0.0);
  for (int d = 0; d < DIMENSIONS; ++d) {
    x[d] = 0.25 * (d + 1);
  }
  GridVertex vertex = GridVertex::create(3, GridVertex::State::Refined, x, adj, false);

  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::Refined));
  validateEquals(vertex.level, 3);
#if PEANO_DEBUG
  validateWithParams2(tarch::la::equals(vertex.getX(), x), vertex.getX(), x);
#endif

  // Adjacent ranks & backup equal to input
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(vertex.adjacentRanks[i], 7);
    validateEquals(vertex.backupOfAdjacentRanks[i], 7);
  }

  validateTrue(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  validateFalse(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  validateFalse(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  validateEquals(vertex.numberOfAdjacentRefinedLocalCells, 0);

  vertex.setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  vertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, false);
  vertex.setFlag(
    GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep,
    vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
  );
  vertex.setFlag(
    GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep,
    vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
  );
  vertex.clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  vertex.clearFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);

  validateFalse(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  validateFalse(vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  validateFalse(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));
}

void peano4::grid::tests::GridVertexTest::testMPIDatatype() {
#ifdef WITH_MPI
  auto&     rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm  comm          = rank.getCommunicator();
  const int me            = rank.getRank();
  const int numberOfRanks = rank.getNumberOfRanks();
  const int tag           = 4321;

  GridVertex sendV;
  sendV.flags = 0;
  sendV.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  sendV.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);

  sendV.setState(GridVertex::State::Refining);
  sendV.level                             = 7 + me; // Make it rank-distinct
  sendV.numberOfAdjacentRefinedLocalCells = 3 + me;
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; ++d) {
    sendV.setX(d, 0.125 * (d + 1) + 0.001 * me);
  }
#endif

  for (std::size_t i = 0; i < sendV.adjacentRanks.size(); ++i) {
    sendV.adjacentRanks[i]         = static_cast<int>(i + 10 + me);
    sendV.backupOfAdjacentRanks[i] = static_cast<int>(i + 20 + me);
  }

  GridVertex recvV;

  if (numberOfRanks == 1) {
    MPI_Sendrecv( // TODO: Capture result for error checking
      &sendV, 1, GridVertex::Datatype, me, tag,
      &recvV, 1, GridVertex::Datatype, me, tag,
      comm, MPI_STATUS_IGNORE
    );
    // GridVertex::send(sendV, me, tag, comm);
    // GridVertex::receive(recvV, me, tag, comm);
  } else {
    const int dest = (me + 1) % numberOfRanks;
    const int src  = (me - 1 + numberOfRanks) % numberOfRanks;

    if ((me % 2) == 0) {
      // Even ranks send then receive
      GridVertex::send(sendV, dest, tag, comm);
      GridVertex::receive(recvV, src, tag, comm);
    } else {
      // Odd ranks receive then send
      GridVertex::receive(recvV, src, tag, comm);
      GridVertex::send(sendV, dest, tag, comm);
    }
  }

  const int expectedSenderRank = (numberOfRanks == 1) ? me : ((me - 1 + numberOfRanks) % numberOfRanks);
  validateEquals(static_cast<int>(recvV.flags), static_cast<int>(sendV.flags));
  validateEquals(static_cast<int>(recvV.state), static_cast<int>(GridVertex::State::Refining));
  validateEquals(recvV.level, 7 + expectedSenderRank);
  validateEquals(recvV.numberOfAdjacentRefinedLocalCells, 3 + expectedSenderRank);
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateNumericalEquals(recvV.getX(d), 0.125 * (d + 1) + 0.001 * expectedSenderRank);
  }
#endif

  for (std::size_t i = 0; i < sendV.adjacentRanks.size(); ++i) {
    validateEquals(recvV.adjacentRanks[i], static_cast<int>(i + 10 + expectedSenderRank));
    validateEquals(recvV.backupOfAdjacentRanks[i], static_cast<int>(i + 20 + expectedSenderRank));
  }
#endif
}
