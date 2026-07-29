// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertexTest.h"

#include "peano4/grid/GridVertex.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

peano4::grid::tests::GridVertexTest::GridVertexTest() :
  TestCase("peano4::grid::tests::GridVertexTest") {}

void peano4::grid::tests::GridVertexTest::run() {
  testMethod(testFlagsAndState);
  testMethod(testCreateGridVertex);
  testMethod(testSendAndReceiveInARing);
}

void peano4::grid::tests::GridVertexTest::testFlagsAndState() {
  GridVertex vertex;
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::New));

  vertex.setState(GridVertex::State::Refined);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::Refined));
  vertex.setState(GridVertex::State::EraseTriggered);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::EraseTriggered));

  vertex.flags = 0;
  for (int bit = 0; bit < 8; bit++) {
    validateFalse(vertex.getFlag(static_cast<std::uint8_t>(1u << bit)));
  }

  vertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  vertex.setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);
  validateTrue(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  vertex.clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  validateFalse(vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue(vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
  validateTrue(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, false);
  validateFalse(vertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));

  const std::string description = vertex.toString();
  validateTrue(description.find("GridVertex") != std::string::npos);
  validateTrue(description.find("EraseTriggered") != std::string::npos);
}

void peano4::grid::tests::GridVertexTest::testCreateGridVertex() {
  std::array<int, TWO_POWER_D> adjacentRanks{};
  adjacentRanks.fill(7);
  tarch::la::Vector<DIMENSIONS, double> x(0.0);
  for (int d = 0; d < DIMENSIONS; d++) {
    x[d] = 0.25 * (d + 1);
  }
  GridVertex vertex = GridVertex::create(3, GridVertex::State::Refined, x, adjacentRanks, false);

  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(GridVertex::State::Refined));
  validateEquals(vertex.level, 3);
#if PEANO_DEBUG
  validateWithParams2(tarch::la::equals(vertex.getX(), x), vertex.getX(), x);
#endif

  for (int i = 0; i < TWO_POWER_D; i++) {
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

void peano4::grid::tests::GridVertexTest::testSendAndReceiveInARing() {
  constexpr int Tag = 4321;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();

  // Every field depends on the rank, so a receiver can predict what its predecessor sent.
  GridVertex sentVertex;
  sentVertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  sentVertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  sentVertex.setState(GridVertex::State::Refining);
  sentVertex.level                             = 7 + localRank;
  sentVertex.numberOfAdjacentRefinedLocalCells = 3 + localRank;
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; d++) {
    sentVertex.setX(d, 0.125 * (d + 1) + 0.001 * localRank);
  }
#endif
  for (std::size_t i = 0; i < sentVertex.adjacentRanks.size(); i++) {
    sentVertex.adjacentRanks[i]         = static_cast<int>(i) + 10 + localRank;
    sentVertex.backupOfAdjacentRanks[i] = static_cast<int>(i) + 20 + localRank;
  }

  const int successor   = (localRank + 1) % numberOfRanks;
  const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

  GridVertex receivedVertex;
  const int  result = tarch::mpi::sendrecv(
    &sentVertex,
    static_cast<int>(sizeof(GridVertex)),
    MPI_BYTE,
    successor,
    Tag,
    &receivedVertex,
    static_cast<int>(sizeof(GridVertex)),
    MPI_BYTE,
    predecessor,
    Tag,
    communicator
  );
  validateEquals(result, MPI_SUCCESS);

  validateEquals(static_cast<int>(receivedVertex.flags), static_cast<int>(sentVertex.flags));
  validateEquals(static_cast<int>(receivedVertex.getState()), static_cast<int>(GridVertex::State::Refining));
  validateEquals(receivedVertex.level, 7 + predecessor);
  validateEquals(receivedVertex.numberOfAdjacentRefinedLocalCells, 3 + predecessor);
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(receivedVertex.getX(d), 0.125 * (d + 1) + 0.001 * predecessor);
  }
#endif

  for (std::size_t i = 0; i < receivedVertex.adjacentRanks.size(); i++) {
    validateEquals(receivedVertex.adjacentRanks[i], static_cast<int>(i) + 10 + predecessor);
    validateEquals(receivedVertex.backupOfAdjacentRanks[i], static_cast<int>(i) + 20 + predecessor);
  }
}
