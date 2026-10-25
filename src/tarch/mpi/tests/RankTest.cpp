// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RankTest.h"

#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

tarch::mpi::tests::RankTest::RankTest() :
  TestCase("tarch::mpi::tests::RankTest") {}

void
tarch::mpi::tests::RankTest::run() {
  testMethod(testRankOnNodeMatchesTheSharedMemorySplit);
  testMethod(testTheNodeMasterHasNodeRankZero);
}

void
tarch::mpi::tests::RankTest::testRankOnNodeMatchesTheSharedMemorySplit() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  MPI_Comm nodeCommunicator{MPI_COMM_NULL};
  tarch::mpi::splitCommunicatorByType(
    MPI_COMM_WORLD,
    MPI_COMM_TYPE_SHARED,
    rank.getRank(),
    MPI_INFO_NULL,
    &nodeCommunicator
  );
  int nodeRank{-1};
  int ranksOnNode{-1};
  tarch::mpi::getRank(nodeCommunicator, &nodeRank);
  tarch::mpi::getNumberOfRanks(nodeCommunicator, &ranksOnNode);
  tarch::mpi::freeCommunicator(&nodeCommunicator);

  validateEquals(rank.getRankOnNode(), nodeRank);
  validateEquals(rank.getNumberOfRanksOnNode(), ranksOnNode);
}

/**
 * The node master has node rank zero. Every node rank lies below the number of ranks on the node.
 */
void
tarch::mpi::tests::RankTest::testTheNodeMasterHasNodeRankZero() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  const bool isTheFirstRankOfItsNode = rank.getRankOnNode() == 0;
  validateEquals(rank.isNodeMaster(), isTheFirstRankOfItsNode);
  validateWithParams1(rank.getRankOnNode() >= 0, rank.getRankOnNode());
  validateWithParams2(
    rank.getRankOnNode() < rank.getNumberOfRanksOnNode(),
    rank.getRankOnNode(),
    rank.getNumberOfRanksOnNode()
  );
}
