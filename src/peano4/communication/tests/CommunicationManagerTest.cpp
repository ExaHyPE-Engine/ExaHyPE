// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManagerTest.h"

#include "peano4/communication/CommunicationManager.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  constexpr int NumberOfProbedTags = 16;
} // namespace

peano4::communication::tests::CommunicationManagerTest::CommunicationManagerTest() :
  TestCase("peano4::communication::tests::CommunicationManagerTest") {}

void
peano4::communication::tests::CommunicationManagerTest::run() {
  testMethod(testTagsAreDistinctAndEveryRankAgrees);
  testMethod(testTheExchangeCommunicatorIsSeparate);
  testMethod(testAPolledTransferCompletes);
}

/**
 * Consecutive reservations return rising tags, and every rank derives the same sequence.
 */
void
peano4::communication::tests::CommunicationManagerTest::testTagsAreDistinctAndEveryRankAgrees() {
  auto&            manager = peano4::communication::CommunicationManager::getInstance();
  std::vector<int> localTags;
  for (int reservation = 0; reservation < NumberOfProbedTags; reservation++) {
    localTags.push_back(manager.reserveTag());
    validate(localTags.back() >= 0);
    if (reservation > 0) {
      validate(localTags.back() > localTags[static_cast<std::size_t>(reservation) - 1]);
    }
  }

  std::vector<int> minimumTags(localTags.size(), 0);
  std::vector<int> maximumTags(localTags.size(), 0);
  tarch::mpi::allreduce(
    localTags.data(),
    minimumTags.data(),
    static_cast<int>(localTags.size()),
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  tarch::mpi::allreduce(
    localTags.data(),
    maximumTags.data(),
    static_cast<int>(localTags.size()),
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  for (std::size_t entry = 0; entry < localTags.size(); entry++) {
    validateEquals(minimumTags[entry], maximumTags[entry]);
    validateEquals(localTags[entry], minimumTags[entry]);
  }
}

/**
 * The exchange communicator differs from the global one and has the same ranks.
 */
void
peano4::communication::tests::CommunicationManagerTest::testTheExchangeCommunicatorIsSeparate() {
  MPI_Comm communicator = peano4::communication::CommunicationManager::getInstance().getCommunicator();
  validate(communicator != MPI_COMM_NULL);
  validate(communicator != tarch::mpi::Rank::getInstance().getCommunicator());

  int rank          = -1;
  int numberOfRanks = -1;
  tarch::mpi::getRank(communicator, &rank);
  tarch::mpi::getNumberOfRanks(communicator, &numberOfRanks);
  validateEquals(rank, tarch::mpi::Rank::getInstance().getRank());
  validateEquals(numberOfRanks, tarch::mpi::Rank::getInstance().getNumberOfRanks());
}

/**
 * The manager polls a registered transfer until it reports completion and then stops polling it.
 */
void
peano4::communication::tests::CommunicationManagerTest::testAPolledTransferCompletes() {
  auto&            manager         = peano4::communication::CommunicationManager::getInstance();
  auto&            progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  std::atomic<int> remainingPolls{3};
  std::atomic<int> numberOfPolls{0};

  manager.registerInFlightTransfer([&remainingPolls, &numberOfPolls]() {
    numberOfPolls.fetch_add(1, std::memory_order_relaxed);
    return remainingPolls.fetch_sub(1, std::memory_order_acq_rel) <= 1;
  });

  while (remainingPolls.load(std::memory_order_acquire) > 0) {
    progressManager.progress();
  }
  progressManager.finishPendingWork();

  validateEquals(numberOfPolls.load(std::memory_order_acquire), 3);
  validateEquals(remainingPolls.load(std::memory_order_acquire), 0);
}
