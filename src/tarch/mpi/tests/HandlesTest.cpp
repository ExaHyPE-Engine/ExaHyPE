// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandlesTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

namespace {
  constexpr int ValuesPerRecord = 2;

  /**
   * Sums the first value and keeps the maximum of the second value of every record. The parameter
   * types follow MPI_User_function, which tarch::mpi::createOperation() takes.
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

tarch::mpi::tests::HandlesTest::HandlesTest() :
  TestCase("tarch::mpi::tests::HandlesTest") {}

void
tarch::mpi::tests::HandlesTest::run() {
  testMethod(testASplitByParityNumbersTheRanksOfEachHalf);
  testMethod(testADuplicateHoldsTheSameRanksAndCommunicates);
  testMethod(testTheTagUpperBoundIsAnAttribute);
  testMethod(testAnInfoObjectIsCreatedAndFreed);
  testMethod(testARecordTypeAndOperationReduceRecords);
}

/**
 * A split by parity numbers the ranks of each half by their global order, and a rank whose colour
 * is undefined receives no communicator, as the file writers rely on.
 */
void
tarch::mpi::tests::HandlesTest::testASplitByParityNumbersTheRanksOfEachHalf() {
  const auto& rank   = tarch::mpi::Rank::getInstance();
  const int   colour = rank.getRank() % 2;

  MPI_Comm half = MPI_COMM_NULL;
  tarch::mpi::splitCommunicator(rank.getCommunicator(), colour, rank.getRank(), &half);
  int rankInHalf  = -1;
  int ranksInHalf = -1;
  tarch::mpi::getRank(half, &rankInHalf);
  tarch::mpi::getNumberOfRanks(half, &ranksInHalf);
  validateEquals(rankInHalf, rank.getRank() / 2);
  validateEquals(ranksInHalf, (rank.getNumberOfRanks() - colour + 1) / 2);
  tarch::mpi::freeCommunicator(&half);
  validate(half == MPI_COMM_NULL);

  MPI_Comm master = MPI_COMM_NULL;
  tarch::mpi::splitCommunicator(
    rank.getCommunicator(),
    rank.isGlobalMaster() ? 0 : MPI_UNDEFINED,
    rank.getRank(),
    &master
  );
  validateEquals(master != MPI_COMM_NULL, rank.isGlobalMaster());
  if (master != MPI_COMM_NULL) {
    int ranksInMaster = -1;
    tarch::mpi::getNumberOfRanks(master, &ranksInMaster);
    validateEquals(ranksInMaster, 1);
    tarch::mpi::freeCommunicator(&master);
  }
}

/**
 * A duplicate is a communicator of its own with the ranks of the original, and a collective
 * completes over it.
 */
void
tarch::mpi::tests::HandlesTest::testADuplicateHoldsTheSameRanksAndCommunicates() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  MPI_Comm duplicate = MPI_COMM_NULL;
  tarch::mpi::duplicateCommunicator(rank.getCommunicator(), &duplicate);
  validate(duplicate != MPI_COMM_NULL);
  validate(duplicate != rank.getCommunicator());
  int rankInDuplicate  = -1;
  int ranksInDuplicate = -1;
  tarch::mpi::getRank(duplicate, &rankInDuplicate);
  tarch::mpi::getNumberOfRanks(duplicate, &ranksInDuplicate);
  validateEquals(rankInDuplicate, rank.getRank());
  validateEquals(ranksInDuplicate, rank.getNumberOfRanks());

  const int one = 1;
  int       sum = 0;
  tarch::mpi::allreduce(&one, &sum, 1, MPI_INT, MPI_SUM, duplicate);
  validateEquals(sum, rank.getNumberOfRanks());
  tarch::mpi::freeCommunicator(&duplicate);
  validate(duplicate == MPI_COMM_NULL);
}

/**
 * Every communicator carries the tag upper bound, which the MPI standard puts at 32767 or above.
 */
void
tarch::mpi::tests::HandlesTest::testTheTagUpperBoundIsAnAttribute() {
  int* tagUpperBound = nullptr;
  int  found         = 0;
  tarch::mpi::getAttribute(
    tarch::mpi::Rank::getInstance().getCommunicator(),
    MPI_TAG_UB,
    static_cast<void*>(&tagUpperBound),
    &found
  );
  validateEquals(found, 1);
  validate(tagUpperBound != nullptr);
  validateWithParams1(*tagUpperBound >= 32767, *tagUpperBound);
}

void
tarch::mpi::tests::HandlesTest::testAnInfoObjectIsCreatedAndFreed() {
  MPI_Info info = MPI_INFO_NULL;
  tarch::mpi::createInfo(&info);
  validate(info != MPI_INFO_NULL);
  tarch::mpi::setInfo(info, "cb_nodes", "1");
  tarch::mpi::freeInfo(&info);
  validate(info == MPI_INFO_NULL);
}

/**
 * A contiguous record type and a reduction operation over whole records reduce one record per
 * rank to the number of ranks and the highest rank.
 */
void
tarch::mpi::tests::HandlesTest::testARecordTypeAndOperationReduceRecords() {
  const auto& rank = tarch::mpi::Rank::getInstance();

  MPI_Datatype recordType = MPI_DATATYPE_NULL;
  MPI_Op       operation  = MPI_OP_NULL;
  tarch::mpi::createContiguousType(ValuesPerRecord, MPI_INT, &recordType);
  tarch::mpi::commitType(&recordType);
  tarch::mpi::createOperation(reduceRecords, true, &operation);

  const std::array<int, ValuesPerRecord> local{1, rank.getRank()};
  std::array<int, ValuesPerRecord>       global{0, 0};
  tarch::mpi::allreduce(local.data(), global.data(), 1, recordType, operation, rank.getCommunicator());
  validateEquals(global[0], rank.getNumberOfRanks());
  validateEquals(global[1], rank.getNumberOfRanks() - 1);

  tarch::mpi::freeOperation(&operation);
  tarch::mpi::freeType(&recordType);
  validate(operation == MPI_OP_NULL);
  validate(recordType == MPI_DATATYPE_NULL);
}
