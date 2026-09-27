// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RankTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

tarch::mpi::tests::RankTest::RankTest() :
  TestCase("tarch::mpi::tests::RankTest") {}

void
tarch::mpi::tests::RankTest::run() {
  testMethod(testRankOnNodeMatchesTheSharedMemorySplit);
  testMethod(testNodeMasterHoldsNodeRankZero);
}

/** The rank on the node and the number of ranks on it are those of a split over the shared memory. */
void
tarch::mpi::tests::RankTest::testRankOnNodeMatchesTheSharedMemorySplit() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  MPI_Comm nodeCommunicator{MPI_COMM_NULL};
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank.getRank(), MPI_INFO_NULL, &nodeCommunicator);
  int nodeRank{-1};
  int ranksOnNode{-1};
  MPI_Comm_rank(nodeCommunicator, &nodeRank);
  MPI_Comm_size(nodeCommunicator, &ranksOnNode);
  MPI_Comm_free(&nodeCommunicator);

  validateEquals(rank.getRankOnNode(), nodeRank);
  validateEquals(rank.getNumberOfRanksOnNode(), ranksOnNode);
}

/** The node master is the rank that carries node rank zero, and every node rank names one of its ranks. */
void
tarch::mpi::tests::RankTest::testNodeMasterHoldsNodeRankZero() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  const bool holdsTheFirstRankOfItsNode = rank.getRankOnNode() == 0;
  validateEquals(rank.isNodeMaster(), holdsTheFirstRankOfItsNode);
  validateWithParams1(rank.getRankOnNode() >= 0, rank.getRankOnNode());
  validateWithParams2(
    rank.getRankOnNode() < rank.getNumberOfRanksOnNode(),
    rank.getRankOnNode(),
    rank.getNumberOfRanksOnNode()
  );
}
