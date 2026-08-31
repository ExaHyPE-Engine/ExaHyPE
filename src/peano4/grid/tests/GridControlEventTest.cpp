// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEventTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

namespace {
  peano4::grid::GridControlEvent createRequest(
    const peano4::grid::GridControlEvent::RefinementControl refinementControl,
    const int                                               level,
    const int                                               firstAxisPosition
  ) {
    peano4::grid::GridControlEvent request;
    request.setRefinementControl(refinementControl);
    request.level = level;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      request.latticeIndex(axis) = firstAxisPosition + axis;
    }
    return request;
  }
} // namespace

peano4::grid::tests::GridControlEventTest::GridControlEventTest() :
  TestCase("peano4::grid::tests::GridControlEventTest") {}

void peano4::grid::tests::GridControlEventTest::run() {
  testMethod(testOperatorEquals);
  testMethod(testOperatorLess);
  testMethod(testOrderIsATotalOrder);
  testMethod(testCanonicalFormIsIndependentOfInputOrder);
  testMethod(testToString);
  testMethod(testMPI);
}

void peano4::grid::tests::GridControlEventTest::testOperatorEquals() {
  const auto reference = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 4, 7);

  auto other = reference;
  validateTrue(reference == other);
  validateTrue(not(reference < other) and not(other < reference));

  other = reference;
  other.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
  validateTrue(not(reference == other));

  other = reference;
  other.level += 1;
  validateTrue(not(reference == other));

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    other = reference;
    other.latticeIndex(axis) += 1;
    validateTrue(not(reference == other));
    validateTrue(reference < other or other < reference);
  }
}

void peano4::grid::tests::GridControlEventTest::testOperatorLess() {
  // A refine request sorts before an erase request.
  {
    const auto refineRequest = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 9, 100);
    const auto eraseRequest  = createRequest(peano4::grid::GridControlEvent::RefinementControl::Erase, 2, 1);
    validateTrue(refineRequest < eraseRequest);
    validateTrue(not(eraseRequest < refineRequest));
  }

  // Within one command the level decides before the lattice position.
  {
    const auto coarseRequest = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 2, 100);
    const auto fineRequest   = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 3, 1);
    validateTrue(coarseRequest < fineRequest);
    validateTrue(not(fineRequest < coarseRequest));
  }

  // Within one level the leading axis decides, even when the trailing axes point the
  // other way.
  {
    auto lowerRequest             = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 4, 0);
    auto higherRequest            = lowerRequest;
    lowerRequest.latticeIndex(0)  = 1;
    higherRequest.latticeIndex(0) = 2;
    if (DIMENSIONS > 1) {
      lowerRequest.latticeIndex(DIMENSIONS - 1)  = 9;
      higherRequest.latticeIndex(DIMENSIONS - 1) = 0;
    }
    validateTrue(lowerRequest < higherRequest);
    validateTrue(not(higherRequest < lowerRequest));
  }
}

void peano4::grid::tests::GridControlEventTest::testOrderIsATotalOrder() {
  std::vector<peano4::grid::GridControlEvent> requests;
  for (const auto refinementControl :
       {peano4::grid::GridControlEvent::RefinementControl::Refine,
        peano4::grid::GridControlEvent::RefinementControl::Erase}) {
    for (int level = 2; level <= 4; level++) {
      for (int firstAxisPosition = 0; firstAxisPosition < 3; firstAxisPosition++) {
        requests.push_back(createRequest(refinementControl, level, firstAxisPosition));
      }
    }
  }

  for (const auto& left : requests) {
    validateTrue(not(left < left));
    for (const auto& right : requests) {
      validateTrue(not(left < right and right < left));
      validateTrue((left == right) == (not(left < right) and not(right < left)));
    }
  }
}

void peano4::grid::tests::GridControlEventTest::testCanonicalFormIsIndependentOfInputOrder() {
  std::vector<peano4::grid::GridControlEvent> ascendingOrder;
  for (int level = 2; level <= 4; level++) {
    for (int firstAxisPosition = 0; firstAxisPosition < 4; firstAxisPosition++) {
      ascendingOrder.push_back(
        createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, level, firstAxisPosition)
      );
    }
  }

  std::vector<peano4::grid::GridControlEvent> shuffledWithDuplicates(ascendingOrder.rbegin(), ascendingOrder.rend());
  shuffledWithDuplicates.insert(shuffledWithDuplicates.end(), ascendingOrder.begin(), ascendingOrder.end());

  std::sort(shuffledWithDuplicates.begin(), shuffledWithDuplicates.end());
  shuffledWithDuplicates.erase(
    std::unique(shuffledWithDuplicates.begin(), shuffledWithDuplicates.end()),
    shuffledWithDuplicates.end()
  );

  std::sort(ascendingOrder.begin(), ascendingOrder.end());
  validateEquals(shuffledWithDuplicates.size(), ascendingOrder.size());
  for (std::size_t requestNumber = 0; requestNumber < ascendingOrder.size(); requestNumber++) {
    validateTrue(shuffledWithDuplicates[requestNumber] == ascendingOrder[requestNumber]);
  }
}

void peano4::grid::tests::GridControlEventTest::testToString() {
  const auto refineRequest = createRequest(peano4::grid::GridControlEvent::RefinementControl::Refine, 3, 5);
  const auto eraseRequest  = createRequest(peano4::grid::GridControlEvent::RefinementControl::Erase, 3, 5);

  validateTrue(refineRequest.toString().find("Refine") != std::string::npos);
  validateTrue(refineRequest.toString().find("Level=3") != std::string::npos);
  validateTrue(refineRequest.toString().find("5") != std::string::npos);
  validateTrue(eraseRequest.toString().find("Erase") != std::string::npos);
}

void peano4::grid::tests::GridControlEventTest::testMPI() {
  constexpr int Tag = 4242;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      myRank        = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();

  auto sentRequest = createRequest(
    (myRank % 2 == 0) ? peano4::grid::GridControlEvent::RefinementControl::Refine
                      : peano4::grid::GridControlEvent::RefinementControl::Erase,
    2 + myRank,
    10 * myRank
  );

  peano4::grid::GridControlEvent receivedRequest;
  MPI_Status                     status;

  if (numberOfRanks == 1) {
    tarch::mpi::sendrecv(
      &sentRequest,
      sizeof(peano4::grid::GridControlEvent),
      MPI_BYTE,
      myRank,
      Tag,
      &receivedRequest,
      sizeof(peano4::grid::GridControlEvent),
      MPI_BYTE,
      myRank,
      Tag,
      communicator,
      &status
    );
  } else {
    const int destination = (myRank + 1) % numberOfRanks;
    const int source      = (myRank - 1 + numberOfRanks) % numberOfRanks;

    if ((myRank % 2) == 0) {
      tarch::mpi::send(&sentRequest, sizeof(peano4::grid::GridControlEvent), MPI_BYTE, destination, Tag, communicator);
      tarch::mpi::receive(
        &receivedRequest,
        sizeof(peano4::grid::GridControlEvent),
        MPI_BYTE,
        source,
        Tag,
        communicator,
        &status
      );
    } else {
      tarch::mpi::receive(
        &receivedRequest,
        sizeof(peano4::grid::GridControlEvent),
        MPI_BYTE,
        source,
        Tag,
        communicator,
        &status
      );
      tarch::mpi::send(&sentRequest, sizeof(peano4::grid::GridControlEvent), MPI_BYTE, destination, Tag, communicator);
    }
  }

  const int  expectedSenderRank = (numberOfRanks == 1) ? myRank : ((myRank - 1 + numberOfRanks) % numberOfRanks);
  const auto expectedRequest    = createRequest(
    (expectedSenderRank % 2 == 0) ? peano4::grid::GridControlEvent::RefinementControl::Refine
                                  : peano4::grid::GridControlEvent::RefinementControl::Erase,
    2 + expectedSenderRank,
    10 * expectedSenderRank
  );

  validateTrue(receivedRequest == expectedRequest);
  validateEquals(status.MPI_SOURCE, expectedSenderRank);
}
