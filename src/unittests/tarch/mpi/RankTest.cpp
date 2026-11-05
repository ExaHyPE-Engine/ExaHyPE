// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/Rank.h"

#include "tarch/mpi/Handles.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

UNIT_TEST("The rank on the node and the number of ranks on the node match a split of the ranks by node") {
  GIVEN("The rank of this process") {
    const auto& rank = tarch::mpi::Rank::getInstance();

    WHEN("Every rank splits the world communicator into one communicator per node") {
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

      THEN("The node rank equals the rank in the node communicator and the ranks on the node equal its size") {
        VALIDATE_EQUALS(rank.getRankOnNode(), nodeRank);
        VALIDATE_EQUALS(rank.getNumberOfRanksOnNode(), ranksOnNode);
      }
    }
  }
}

UNIT_TEST("The node master is the rank with node rank zero and every node rank lies below the ranks on the node") {
  GIVEN("The rank of this process") {
    const auto& rank = tarch::mpi::Rank::getInstance();

    WHEN("The rank reports its node rank, the number of ranks on its node and whether it is the node master") {
      const int  rankOnNode              = rank.getRankOnNode();
      const int  ranksOnNode             = rank.getNumberOfRanksOnNode();
      const bool isNodeMaster            = rank.isNodeMaster();
      const bool isTheFirstRankOfItsNode = rankOnNode == 0;

      THEN("Only node rank zero is the node master and node ranks are not negative and below the ranks on the node") {
        VALIDATE_EQUALS(isNodeMaster, isTheFirstRankOfItsNode);
        VALIDATE(rankOnNode >= 0, rankOnNode);
        VALIDATE(rankOnNode < ranksOnNode, rankOnNode, ranksOnNode);
      }
    }
  }
}
