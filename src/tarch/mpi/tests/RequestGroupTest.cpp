// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RequestGroupTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/mpi/RequestGroup.h"

namespace {
  /** Loopback transfers let the case run with any number of ranks. */
  int groupTestTag() {
    static const int tag = tarch::mpi::Rank::reserveFreeTag("tarch::mpi::tests::RequestGroupTest", 4);
    return tag;
  }
} // namespace

tarch::mpi::tests::RequestGroupTest::RequestGroupTest() :
  tarch::tests::TestCase("tarch::mpi::tests::RequestGroupTest") {}

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

  tarch::mpi::RequestGroup requestGroup;
  for (int transferIndex = 0; transferIndex < NumberOfTransfers; transferIndex++) {
    outgoing[transferIndex]        = 100 + transferIndex;
    sendRequests[transferIndex]    = MPI_REQUEST_NULL;
    receiveRequests[transferIndex] = MPI_REQUEST_NULL;
    tarch::mpi::irecv(
      &incoming[transferIndex],
      1,
      MPI_INT,
      rank,
      groupTestTag() + transferIndex,
      communicator,
      &receiveRequests[transferIndex]
    );
    tarch::mpi::isend(
      &outgoing[transferIndex],
      1,
      MPI_INT,
      rank,
      groupTestTag() + transferIndex,
      communicator,
      &sendRequests[transferIndex]
    );
    requestGroup.add(&sendRequests[transferIndex]);
    requestGroup.add(&receiveRequests[transferIndex]);
  }

  requestGroup.waitAll("tarch::mpi::tests::RequestGroupTest", "testWaitAllCompletesPostedTransfers()");

  for (int transferIndex = 0; transferIndex < NumberOfTransfers; transferIndex++) {
    validateEqualsWithParams1(incoming[transferIndex], outgoing[transferIndex], transferIndex);
  }
  validate(requestGroup.testAll());
}

/** An empty group is complete by definition. */
void tarch::mpi::tests::RequestGroupTest::testEmptyGroupNeedsNoWait() {
  tarch::mpi::RequestGroup requestGroup;
  validate(requestGroup.testAll());
  requestGroup.waitAll("tarch::mpi::tests::RequestGroupTest", "testEmptyGroupNeedsNoWait()");
}

/** clear() empties the set, so the group can drain a fresh round of requests. */
void tarch::mpi::tests::RequestGroupTest::testClearedGroupStartsOver() {
  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  int         outgoing       = 41;
  int         incoming       = 0;
  MPI_Request sendRequest    = MPI_REQUEST_NULL;
  MPI_Request receiveRequest = MPI_REQUEST_NULL;

  tarch::mpi::RequestGroup requestGroup;
  tarch::mpi::irecv(&incoming, 1, MPI_INT, rank, groupTestTag(), communicator, &receiveRequest);
  tarch::mpi::isend(&outgoing, 1, MPI_INT, rank, groupTestTag(), communicator, &sendRequest);
  requestGroup.add(&sendRequest);
  requestGroup.add(&receiveRequest);
  requestGroup.waitAll("tarch::mpi::tests::RequestGroupTest", "testClearedGroupStartsOver()");
  validateEquals(incoming, outgoing);

  requestGroup.clear();
  validate(requestGroup.testAll());

  outgoing       = 42;
  incoming       = 0;
  sendRequest    = MPI_REQUEST_NULL;
  receiveRequest = MPI_REQUEST_NULL;
  tarch::mpi::irecv(&incoming, 1, MPI_INT, rank, groupTestTag(), communicator, &receiveRequest);
  tarch::mpi::isend(&outgoing, 1, MPI_INT, rank, groupTestTag(), communicator, &sendRequest);
  requestGroup.add(&sendRequest);
  requestGroup.add(&receiveRequest);
  requestGroup.waitAll("tarch::mpi::tests::RequestGroupTest", "testClearedGroupStartsOver()");
  validateEquals(incoming, 42);
}
