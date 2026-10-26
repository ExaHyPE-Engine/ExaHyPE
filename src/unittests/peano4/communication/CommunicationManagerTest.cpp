// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/communication/CommunicationManager.h"

#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

namespace {
  constexpr int NumberOfProbedTags = 16;
} // namespace

UNIT_TEST("Consecutive tag reservations return rising tags and every rank reserves the same sequence") {
  GIVEN("The communication manager of every rank") {
    auto& manager = peano4::communication::CommunicationManager::getInstance();

    WHEN("Every rank reserves the probed tags and reduces every tag to its minimum and its maximum over the ranks") {
      std::vector<int> localTags;
      for (int reservation = 0; reservation < NumberOfProbedTags; reservation++) {
        localTags.push_back(manager.reserveTag());
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

      THEN("Every tag is non-negative and rises above its predecessor and every rank reserves the same tags") {
        for (std::size_t reservation = 0; reservation < localTags.size(); reservation++) {
          VALIDATE(localTags[reservation] >= 0);
          if (reservation > 0) {
            VALIDATE(localTags[reservation] > localTags[reservation - 1]);
          }
        }
        for (std::size_t entry = 0; entry < localTags.size(); entry++) {
          VALIDATE_EQUALS(minimumTags[entry], maximumTags[entry]);
          VALIDATE_EQUALS(localTags[entry], minimumTags[entry]);
        }
      }
    }
  }
}

UNIT_TEST("The exchange communicator differs from the global one and has the same ranks") {
  GIVEN("The exchange communicator of the communication manager and the global communicator") {
    const MPI_Comm communicator       = peano4::communication::CommunicationManager::getInstance().getCommunicator();
    const MPI_Comm globalCommunicator = tarch::mpi::Rank::getInstance().getCommunicator();

    WHEN("Every rank reads its rank and the number of ranks of a valid exchange communicator") {
      int rank          = -1;
      int numberOfRanks = -1;
      if (communicator != MPI_COMM_NULL) {
        tarch::mpi::getRank(communicator, &rank);
        tarch::mpi::getNumberOfRanks(communicator, &numberOfRanks);
      }

      THEN("The exchange communicator is valid, differs from the global one and numbers the ranks as it does") {
        VALIDATE(communicator != MPI_COMM_NULL);
        VALIDATE(communicator != globalCommunicator);
        VALIDATE_EQUALS(rank, tarch::mpi::Rank::getInstance().getRank());
        VALIDATE_EQUALS(numberOfRanks, tarch::mpi::Rank::getInstance().getNumberOfRanks());
      }
    }
  }
}

UNIT_TEST("The manager polls a registered transfer until it reports completion and then stops polling it") {
  GIVEN("The communication manager, the progress engine and a transfer whose third poll reports completion") {
    auto&            manager         = peano4::communication::CommunicationManager::getInstance();
    auto&            progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
    std::atomic<int> remainingPolls{3};
    std::atomic<int> numberOfPolls{0};

    WHEN("The manager registers the transfer and the engine progresses until it completes and finishes its work") {
      manager.registerInFlightTransfer([&remainingPolls, &numberOfPolls]() {
        numberOfPolls.fetch_add(1, std::memory_order_relaxed);
        return remainingPolls.fetch_sub(1, std::memory_order_acq_rel) <= 1;
      });

      while (remainingPolls.load(std::memory_order_acquire) > 0) {
        progressManager.progress();
      }
      progressManager.finishPendingWork();

      THEN("The engine polls the transfer three times and uses up the remaining polls") {
        VALIDATE_EQUALS(numberOfPolls.load(std::memory_order_acquire), 3);
        VALIDATE_EQUALS(remainingPolls.load(std::memory_order_acquire), 0);
      }
    }
  }
}
