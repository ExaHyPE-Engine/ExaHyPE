// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/Handles.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

namespace {
  constexpr int ValuesPerRecord = 2;

  /**
   * Sums the first values and keeps the maximum of the second values of every record. The
   * parameter types follow MPI_User_function.
   */
  void reduceRecords(
    void*                          incoming,
    void*                          accumulating,
    int*                           numberOfRecords, // NOLINT(readability-non-const-parameter)
    [[maybe_unused]] MPI_Datatype* datatype
  ) {
    const auto* const source = static_cast<const int*>(incoming);
    auto* const       target = static_cast<int*>(accumulating);
    for (int record = 0; record < *numberOfRecords; record++) {
      target[record * ValuesPerRecord] += source[record * ValuesPerRecord];
      target[record * ValuesPerRecord + 1] =
        std::max(target[record * ValuesPerRecord + 1], source[record * ValuesPerRecord + 1]);
    }
  }
} // namespace

UNIT_TEST("A split by parity numbers the ranks of each half by their global order") {
  GIVEN("The rank of this process and the parity of its rank") {
    const auto& rank   = tarch::mpi::Rank::getInstance();
    const int   colour = rank.getRank() % 2;

    WHEN("Every rank splits the communicator by the parity of its rank and frees its half afterwards") {
      MPI_Comm half = MPI_COMM_NULL;
      tarch::mpi::splitCommunicator(rank.getCommunicator(), colour, rank.getRank(), &half);
      int rankInHalf  = -1;
      int ranksInHalf = -1;
      tarch::mpi::getRank(half, &rankInHalf);
      tarch::mpi::getNumberOfRanks(half, &ranksInHalf);
      tarch::mpi::freeCommunicator(&half);

      THEN("The rank in the half is half the global rank, every rank of the parity joins and the free resets it") {
        VALIDATE_EQUALS(rankInHalf, rank.getRank() / 2);
        VALIDATE_EQUALS(ranksInHalf, (rank.getNumberOfRanks() - colour + 1) / 2);
        VALIDATE(half == MPI_COMM_NULL);
      }
    }
  }
}

UNIT_TEST("A split returns MPI_COMM_NULL to a rank whose colour is MPI_UNDEFINED") {
  GIVEN("The rank of this process") {
    const auto& rank = tarch::mpi::Rank::getInstance();

    WHEN("The global master splits off a communicator of its own and every other rank passes MPI_UNDEFINED") {
      MPI_Comm master = MPI_COMM_NULL;
      tarch::mpi::splitCommunicator(
        rank.getCommunicator(),
        rank.isGlobalMaster() ? 0 : MPI_UNDEFINED,
        rank.getRank(),
        &master
      );
      const bool receivesACommunicator = master != MPI_COMM_NULL;
      int        ranksInMaster         = 0;
      if (receivesACommunicator) {
        tarch::mpi::getNumberOfRanks(master, &ranksInMaster);
        tarch::mpi::freeCommunicator(&master);
      }

      THEN("Only the global master receives a communicator and the communicator spans this one rank") {
        VALIDATE_EQUALS(receivesACommunicator, rank.isGlobalMaster());
        VALIDATE_EQUALS(ranksInMaster, rank.isGlobalMaster() ? 1 : 0);
      }
    }
  }
}

UNIT_TEST("A duplicate is a new communicator with the ranks of the original and a collective completes over it") {
  GIVEN("The rank of this process") {
    const auto& rank = tarch::mpi::Rank::getInstance();

    WHEN("Every rank duplicates the communicator, sums one over the duplicate and frees the duplicate") {
      MPI_Comm duplicate = MPI_COMM_NULL;
      tarch::mpi::duplicateCommunicator(rank.getCommunicator(), &duplicate);
      const bool isNewCommunicator = duplicate != MPI_COMM_NULL and duplicate != rank.getCommunicator();
      int        rankInDuplicate   = -1;
      int        ranksInDuplicate  = -1;
      tarch::mpi::getRank(duplicate, &rankInDuplicate);
      tarch::mpi::getNumberOfRanks(duplicate, &ranksInDuplicate);
      const int one = 1;
      int       sum = 0;
      tarch::mpi::allreduce(&one, &sum, 1, MPI_INT, MPI_SUM, duplicate);
      tarch::mpi::freeCommunicator(&duplicate);

      THEN("The duplicate keeps rank and size, the sum counts every rank and the free resets the handle") {
        VALIDATE(isNewCommunicator);
        VALIDATE_EQUALS(rankInDuplicate, rank.getRank());
        VALIDATE_EQUALS(ranksInDuplicate, rank.getNumberOfRanks());
        VALIDATE_EQUALS(sum, rank.getNumberOfRanks());
        VALIDATE(duplicate == MPI_COMM_NULL);
      }
    }
  }
}

UNIT_TEST("The communicator returns the tag upper bound as an attribute of at least 32767") {
  GIVEN("The communicator of this process") {
    const MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();

    WHEN("The rank reads the attribute MPI_TAG_UB") {
      int* tagUpperBound = nullptr;
      int  found         = 0;
      tarch::mpi::getAttribute(communicator, MPI_TAG_UB, static_cast<void*>(&tagUpperBound), &found);

      THEN("The attribute exists and is at least 32767, the minimum of the MPI standard") {
        VALIDATE_EQUALS(found, 1);
        VALIDATE(tagUpperBound != nullptr);
        VALIDATE(*tagUpperBound >= 32767, *tagUpperBound);
      }
    }
  }
}

UNIT_TEST("The function createInfo() returns an info object that takes a key and freeInfo() resets the handle") {
  GIVEN("A null info handle") {
    MPI_Info info = MPI_INFO_NULL;

    WHEN("The function createInfo() creates an info object, setInfo() sets a key and freeInfo() frees the object") {
      tarch::mpi::createInfo(&info);
      const bool isCreated = info != MPI_INFO_NULL;
      tarch::mpi::setInfo(info, "cb_nodes", "1");
      tarch::mpi::freeInfo(&info);

      THEN("The handle names an object until freeInfo() resets it to MPI_INFO_NULL") {
        VALIDATE(isCreated);
        VALIDATE(info == MPI_INFO_NULL);
      }
    }
  }
}

UNIT_TEST("A contiguous record type and an operation over whole records reduce one record per rank") {
  GIVEN("A record type of two integers, an operation over records and the record of one and the rank") {
    const auto&  rank       = tarch::mpi::Rank::getInstance();
    MPI_Datatype recordType = MPI_DATATYPE_NULL;
    MPI_Op       operation  = MPI_OP_NULL;
    tarch::mpi::createContiguousType(ValuesPerRecord, MPI_INT, &recordType);
    tarch::mpi::commitType(&recordType);
    tarch::mpi::createOperation(reduceRecords, true, &operation);
    const std::array<int, ValuesPerRecord> local{1, rank.getRank()};

    WHEN("Every rank reduces its record over all ranks and frees the operation and the type") {
      std::array<int, ValuesPerRecord> global{0, 0};
      tarch::mpi::allreduce(local.data(), global.data(), 1, recordType, operation, rank.getCommunicator());
      tarch::mpi::freeOperation(&operation);
      tarch::mpi::freeType(&recordType);

      THEN("The reduction returns the number of ranks and the highest rank and both frees reset their handles") {
        VALIDATE_EQUALS(global[0], rank.getNumberOfRanks());
        VALIDATE_EQUALS(global[1], rank.getNumberOfRanks() - 1);
        VALIDATE(operation == MPI_OP_NULL);
        VALIDATE(recordType == MPI_DATATYPE_NULL);
      }
    }
  }
}
