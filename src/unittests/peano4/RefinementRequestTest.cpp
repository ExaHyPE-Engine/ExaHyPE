// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/RefinementRequest.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

namespace {
  peano4::RefinementRequest getRequest(
    const peano4::RefinementRequest::RefinementControl refinementControl,
    const int                                          level,
    const int                                          firstAxisPosition
  ) {
    peano4::RefinementRequest request{};
    request.setRefinementControl(refinementControl);
    request.level = level;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      request.latticeIndex(axis) = firstAxisPosition + axis;
    }
    return request;
  }
} // namespace

UNIT_TEST("Two requests are equal when their control, their level and their lattice position agree") {
  GIVEN("A refine request, its copy and copies with the erase control, one level more or one step along an axis") {
    const auto reference = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 4, 7);
    const auto copy      = reference;
    auto       erase     = reference;
    erase.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    auto finer = reference;
    finer.level += 1;
    std::vector<peano4::RefinementRequest> movedAlongAxis(DIMENSIONS, reference);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      movedAlongAxis[axis].latticeIndex(axis) += 1;
    }

    THEN("The request equals its copy and the two are incomparable") {
      VALIDATE(reference == copy);
      VALIDATE(not(reference < copy) and not(copy < reference));
    }

    THEN("The request differs from every changed copy and every moved copy sorts on one side of the request") {
      VALIDATE(not(reference == erase));
      VALIDATE(not(reference == finer));
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        VALIDATE(not(reference == movedAlongAxis[axis]));
        VALIDATE(reference < movedAlongAxis[axis] or movedAlongAxis[axis] < reference);
      }
    }
  }
}

UNIT_TEST("A refine sorts before an erase, a coarse level before a finer one and the first axis decides in a level") {
  GIVEN("A refine request on level nine and an erase request on level two") {
    const auto refineRequest = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 9, 100);
    const auto eraseRequest  = getRequest(peano4::RefinementRequest::RefinementControl::Erase, 2, 1);

    THEN("The refine request sorts before the erase request") {
      VALIDATE(refineRequest < eraseRequest);
      VALIDATE(not(eraseRequest < refineRequest));
    }
  }

  GIVEN("A refine request on level two and a refine request on level three at a lower lattice position") {
    const auto coarseRequest = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 2, 100);
    const auto fineRequest   = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 3, 1);

    THEN("The coarse request sorts before the fine request") {
      VALIDATE(coarseRequest < fineRequest);
      VALIDATE(not(fineRequest < coarseRequest));
    }
  }

  GIVEN("Two refine requests on level four, the lower one lower along the first axis and higher along the last axis") {
    auto lowerRequest             = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 4, 0);
    auto higherRequest            = lowerRequest;
    lowerRequest.latticeIndex(0)  = 1;
    higherRequest.latticeIndex(0) = 2;
    if (DIMENSIONS > 1) {
      lowerRequest.latticeIndex(DIMENSIONS - 1)  = 9;
      higherRequest.latticeIndex(DIMENSIONS - 1) = 0;
    }

    THEN("The request lower along the first axis sorts first") {
      VALIDATE(lowerRequest < higherRequest);
      VALIDATE(not(higherRequest < lowerRequest));
    }
  }
}

UNIT_TEST("The order of requests is a total order and two requests are equal when they are incomparable") {
  GIVEN("The refine and the erase requests on the levels two to four at three lattice positions each") {
    std::vector<peano4::RefinementRequest> requests;
    for (const auto refinementControl :
         {peano4::RefinementRequest::RefinementControl::Refine, peano4::RefinementRequest::RefinementControl::Erase}) {
      for (int level = 2; level <= 4; level++) {
        for (int firstAxisPosition = 0; firstAxisPosition < 3; firstAxisPosition++) {
          requests.push_back(getRequest(refinementControl, level, firstAxisPosition));
        }
      }
    }

    THEN("The order is irreflexive and asymmetric and two requests are equal when they are incomparable") {
      for (const auto& left : requests) {
        VALIDATE(not(left < left));
        for (const auto& right : requests) {
          VALIDATE(not(left < right and right < left));
          VALIDATE((left == right) == (not(left < right) and not(right < left)));
        }
      }
    }
  }
}

UNIT_TEST("The sorted form of a request set without duplicates is independent of the input order") {
  GIVEN("Refine requests in ascending order and the same requests in reverse order followed by them again") {
    std::vector<peano4::RefinementRequest> ascendingOrder;
    for (int level = 2; level <= 4; level++) {
      for (int firstAxisPosition = 0; firstAxisPosition < 4; firstAxisPosition++) {
        ascendingOrder.push_back(
          getRequest(peano4::RefinementRequest::RefinementControl::Refine, level, firstAxisPosition)
        );
      }
    }
    std::vector<peano4::RefinementRequest> shuffledWithDuplicates(ascendingOrder.rbegin(), ascendingOrder.rend());
    shuffledWithDuplicates.insert(shuffledWithDuplicates.end(), ascendingOrder.begin(), ascendingOrder.end());

    WHEN("The test sorts both lists and removes the duplicates of the second list") {
      std::sort(shuffledWithDuplicates.begin(), shuffledWithDuplicates.end());
      shuffledWithDuplicates.erase(
        std::unique(shuffledWithDuplicates.begin(), shuffledWithDuplicates.end()),
        shuffledWithDuplicates.end()
      );
      std::sort(ascendingOrder.begin(), ascendingOrder.end());

      THEN("Both lists contain the same requests in the same order") {
        VALIDATE_EQUALS(shuffledWithDuplicates.size(), ascendingOrder.size());
        for (std::size_t requestNumber = 0; requestNumber < ascendingOrder.size(); requestNumber++) {
          VALIDATE(shuffledWithDuplicates[requestNumber] == ascendingOrder[requestNumber]);
        }
      }
    }
  }
}

UNIT_TEST("The description of a request contains its control, its level and its lattice position") {
  GIVEN("A refine request and an erase request on level three at the lattice position five plus the axis") {
    const auto refineRequest = getRequest(peano4::RefinementRequest::RefinementControl::Refine, 3, 5);
    const auto eraseRequest  = getRequest(peano4::RefinementRequest::RefinementControl::Erase, 3, 5);

    WHEN("Both requests write their description") {
      const std::string refineDescription = refineRequest.toString();
      const std::string eraseDescription  = eraseRequest.toString();

      THEN("The descriptions contain the control of their request, the level three and the position five") {
        VALIDATE(refineDescription.find("Refine") != std::string::npos);
        VALIDATE(refineDescription.find("Level=3") != std::string::npos);
        VALIDATE(refineDescription.find("5") != std::string::npos);
        VALIDATE(eraseDescription.find("Erase") != std::string::npos);
      }
    }
  }
}

UNIT_TEST("A request travels between the ranks as raw bytes and arrives unchanged") {
  GIVEN("A request on every rank derived from the rank and the request the previous rank in the ring derives") {
    constexpr int Tag = 4242;

    const auto&    rank          = tarch::mpi::Rank::getInstance();
    const MPI_Comm communicator  = rank.getCommunicator();
    const int      localRank     = rank.getRank();
    const int      numberOfRanks = rank.getNumberOfRanks();

    const auto sentRequest = getRequest(
      (localRank % 2 == 0) ? peano4::RefinementRequest::RefinementControl::Refine
                           : peano4::RefinementRequest::RefinementControl::Erase,
      2 + localRank,
      10 * localRank
    );

    const int expectedSenderRank = (numberOfRanks == 1) ? localRank : ((localRank - 1 + numberOfRanks) % numberOfRanks);
    const auto expectedRequest   = getRequest(
      (expectedSenderRank % 2 == 0) ? peano4::RefinementRequest::RefinementControl::Refine
                                    : peano4::RefinementRequest::RefinementControl::Erase,
      2 + expectedSenderRank,
      10 * expectedSenderRank
    );

    WHEN("Every rank sends its request to the next rank in the ring and receives the request of the previous one") {
      peano4::RefinementRequest receivedRequest;
      MPI_Status                status;

      if (numberOfRanks == 1) {
        tarch::mpi::sendrecv(
          &sentRequest,
          sizeof(peano4::RefinementRequest),
          MPI_BYTE,
          localRank,
          Tag,
          &receivedRequest,
          sizeof(peano4::RefinementRequest),
          MPI_BYTE,
          localRank,
          Tag,
          communicator,
          &status
        );
      } else {
        const int destination = (localRank + 1) % numberOfRanks;
        const int source      = (localRank - 1 + numberOfRanks) % numberOfRanks;

        if ((localRank % 2) == 0) {
          tarch::mpi::send(&sentRequest, sizeof(peano4::RefinementRequest), MPI_BYTE, destination, Tag, communicator);
          tarch::mpi::receive(
            &receivedRequest,
            sizeof(peano4::RefinementRequest),
            MPI_BYTE,
            source,
            Tag,
            communicator,
            &status
          );
        } else {
          tarch::mpi::receive(
            &receivedRequest,
            sizeof(peano4::RefinementRequest),
            MPI_BYTE,
            source,
            Tag,
            communicator,
            &status
          );
          tarch::mpi::send(&sentRequest, sizeof(peano4::RefinementRequest), MPI_BYTE, destination, Tag, communicator);
        }
      }

      THEN("The received request equals the request of the previous rank and the status reports that rank as source") {
        VALIDATE(receivedRequest == expectedRequest);
        VALIDATE_EQUALS(status.MPI_SOURCE, expectedSenderRank);
      }
    }
  }
}
