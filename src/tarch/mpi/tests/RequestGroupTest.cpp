// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RequestGroupTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/mpi/RequestGroup.h"

namespace {
  /** Every transfer goes to the own rank, so the case runs on any rank count. */
  int groupTestTag() {
    static int tag = tarch::mpi::Rank::reserveFreeTag("tarch::mpi::tests::RequestGroupTest", 4);
    return tag;
  }
} // namespace

tarch::mpi::tests::RequestGroupTest::RequestGroupTest() :
  TestCase("tarch::mpi::tests::RequestGroupTest") {}

void tarch::mpi::tests::RequestGroupTest::run() {
  testMethod(testWaitAllCompletesPostedTransfers);
  testMethod(testEmptyGroupNeedsNoWait);
  testMethod(testClearedGroupStartsOver);
}

/** waitAll() drains every added request, and a completed set keeps reporting completion. */
void tarch::mpi::tests::RequestGroupTest::testWaitAllCompletesPostedTransfers() {
  constexpr int NumberOfTransfers = 4;

  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  std::array<int, NumberOfTransfers>         outgoing{};
  std::array<int, NumberOfTransfers>         incoming{};
  std::array<MPI_Request, NumberOfTransfers> sendRequests{};
  std::array<MPI_Request, NumberOfTransfers> receiveRequests{};

  tarch::mpi::RequestGroup group;
  for (int transfer = 0; transfer < NumberOfTransfers; transfer++) {
    outgoing[transfer]        = 100 + transfer;
    sendRequests[transfer]    = MPI_REQUEST_NULL;
    receiveRequests[transfer] = MPI_REQUEST_NULL;
    tarch::mpi::irecv(
      &incoming[transfer],
      1,
      MPI_INT,
      rank,
      groupTestTag() + transfer,
      communicator,
      &receiveRequests[transfer]
    );
    tarch::mpi::isend(
      &outgoing[transfer],
      1,
      MPI_INT,
      rank,
      groupTestTag() + transfer,
      communicator,
      &sendRequests[transfer]
    );
    group.add(&sendRequests[transfer]);
    group.add(&receiveRequests[transfer]);
  }

  group.waitAll("tarch::mpi::tests::RequestGroupTest", "testWaitAllCompletesPostedTransfers()");

  for (int transfer = 0; transfer < NumberOfTransfers; transfer++) {
    validateEqualsWithParams1(incoming[transfer], outgoing[transfer], transfer);
  }
  validate(group.testAll());
}

/** An empty group is complete by definition. */
void tarch::mpi::tests::RequestGroupTest::testEmptyGroupNeedsNoWait() {
  tarch::mpi::RequestGroup group;
  validate(group.testAll());
  group.waitAll("tarch::mpi::tests::RequestGroupTest", "testEmptyGroupNeedsNoWait()");
}

/** clear() empties the set, so the group can drain a fresh round of requests. */
void tarch::mpi::tests::RequestGroupTest::testClearedGroupStartsOver() {
  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  int         outgoing       = 41;
  int         incoming       = 0;
  MPI_Request sendRequest    = MPI_REQUEST_NULL;
  MPI_Request receiveRequest = MPI_REQUEST_NULL;

  tarch::mpi::RequestGroup group;
  tarch::mpi::irecv(&incoming, 1, MPI_INT, rank, groupTestTag(), communicator, &receiveRequest);
  tarch::mpi::isend(&outgoing, 1, MPI_INT, rank, groupTestTag(), communicator, &sendRequest);
  group.add(&sendRequest);
  group.add(&receiveRequest);
  group.waitAll("tarch::mpi::tests::RequestGroupTest", "testClearedGroupStartsOver()");
  validateEquals(incoming, outgoing);

  group.clear();
  validate(group.testAll());

  outgoing       = 42;
  incoming       = 0;
  sendRequest    = MPI_REQUEST_NULL;
  receiveRequest = MPI_REQUEST_NULL;
  tarch::mpi::irecv(&incoming, 1, MPI_INT, rank, groupTestTag(), communicator, &receiveRequest);
  tarch::mpi::isend(&outgoing, 1, MPI_INT, rank, groupTestTag(), communicator, &sendRequest);
  group.add(&sendRequest);
  group.add(&receiveRequest);
  group.waitAll("tarch::mpi::tests::RequestGroupTest", "testClearedGroupStartsOver()");
  validateEquals(incoming, 42);
}
